#import <Metal/Metal.h>
#import <JavascriptCore/JavascriptCore.h>
#import <vector>

#define HYDRA_OUT (1+1)
#define HYDRA_IN 4
#define HYDRA_UNIFORM_TIME 0
#define HYDRA_UNIFORM_RESOLUTION 1
#define HYDRA_UNIFORM_MOUSE 2
#define HYDRA_OFFSET_UNIFORM 3

namespace Hydra {

    JSContext *jscontext = nil;

    enum UniformType {
        FloatType = 0,
        FunctionType,
    };
}

class HydraComputeShader : public ComputeShaderBase<unsigned int> {

    protected:
    
        double startTime = CFAbsoluteTimeGetCurrent();
    
        NSMutableDictionary *_uniforms = [NSMutableDictionary dictionary];
        NSMutableArray *_uniformsKey = nil;

        std::vector<Hydra::UniformType> _uniformsType;
        NSMutableArray *_uniformsValue = [NSMutableArray array];
    
        void setupArgumentEncoderWithBuffer() {
            
            this->_argumentEncoder = [this->_function newArgumentEncoderWithBufferIndex:0];
            this->_argumentEncoderBuffer = MTLUtils::newBuffer(this->_device,sizeof(float)*[this->_argumentEncoder encodedLength]);
            [this->_argumentEncoder setArgumentBuffer:this->_argumentEncoderBuffer offset:0];
        
            if(this->_arguments.size()==0) {
                this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)));
                this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)*2));
                this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)*2));
            }
            
            this->set(this->_arguments[HYDRA_UNIFORM_TIME],0);
            this->set(this->_arguments[HYDRA_UNIFORM_RESOLUTION],this->_width,this->_height);
            this->set(this->_arguments[HYDRA_UNIFORM_MOUSE],0,0);

            [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_TIME] offset:0 atIndex:0];
            [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_RESOLUTION] offset:0 atIndex:1];
            [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_MOUSE] offset:0 atIndex:2];

       
            for(int k=0; k<[this->_uniformsKey count]; k++) {
                
                if([this->_uniformsValue count]<=k) { // add
                    
                    [this->_uniformsValue addObject:this->_uniforms[this->_uniformsKey[k]]];
                    this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)));

                }
                else { // overwrite
                    this->_uniformsValue[k] = this->_uniforms[this->_uniformsKey[k]];
                }
                
                [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_OFFSET_UNIFORM+k] offset:0 atIndex:HYDRA_OFFSET_UNIFORM+k];

            }
                                
            for(int k=0; k<[this->_uniformsKey count]; k++) {
                
                Hydra::UniformType type = Hydra::UniformType::FloatType;
                
                if([NSStringFromClass([_uniformsValue[k] class]) compare:@"__NSCFString"]==NSOrderedSame) {
                    type = Hydra::UniformType::FunctionType;
                }
                else {
                    this->set(this->_arguments[HYDRA_OFFSET_UNIFORM+k],[this->_uniformsValue[k] floatValue]);
                }
                
                if(this->_uniformsType.size()<=k) { // add
                    this->_uniformsType.push_back(type);
                }
                else { // overwrite
                    this->_uniformsType[k] = type;
                }
            }
        }
    
        void sortUniformsKey() {
            this->_uniformsKey = (NSMutableArray *)[(NSArray *)this->_uniformsKey sortedArrayUsingComparator:^NSComparisonResult(NSString *s1,NSString *s2) {
                int n1 = [[s1 componentsSeparatedByString:@"_"][1] intValue];
                int n2 = [[s2 componentsSeparatedByString:@"_"][1] intValue];
                if(n1<n2) return (NSComparisonResult)NSOrderedAscending;
                else if(n1>n2) return (NSComparisonResult)NSOrderedDescending;
                else return (NSComparisonResult)NSOrderedSame;
            }];
        }
        
    public:
    
        void set(id<MTLBuffer> buffer,float x) {
            float *f = (float *)[buffer contents];
            f[0] = x;
        }

        void set(id<MTLBuffer> buffer,float x,float y) {
            float *f = (float *)[buffer contents];
            f[0] = x;
            f[1] = y;
        }
    
        float *get(id<MTLBuffer> buffer) {
            return ((float *)[buffer contents]);
        }
    
        float get(id<MTLBuffer> buffer,int n) {
            return ((float *)[buffer contents])[n];
        }
        
        unsigned int *exec() {
            
            if(this->init()) {
                
                MTLUtils::replace(this->_texture[1],this->_buffer[0]->bytes(),this->_buffer[0]->width(),this->_buffer[0]->height(),this->_buffer[0]->rowBytes());

                this->set(this->_arguments[HYDRA_UNIFORM_TIME],CFAbsoluteTimeGetCurrent()-this->startTime);

                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"time=%f;",this->get(this->_arguments[HYDRA_UNIFORM_TIME],0)]];
                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"resolution={x:%d,y:%d};",this->_width,this->_height]];
                float *mouseBuffer = this->get(this->_arguments[HYDRA_UNIFORM_MOUSE]);
                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"mouse={x:%f,y:%f};",mouseBuffer[0],mouseBuffer[1]]];
                
                for(int k=0; k<this->_uniformsType.size(); k++) {
                    Hydra::UniformType type = this->_uniformsType[k];
                    if(type==Hydra::UniformType::FunctionType) {
                        
                        this->set(this->_arguments[HYDRA_OFFSET_UNIFORM+k],[[Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"(%@)();",this->_uniformsValue[k]]] toDouble]);
                    }
                }
                
                ComputeShaderBase::update();
                this->_buffer[0]->getBytes(this->_texture[0],true);
            }
            return (unsigned int *)this->_buffer[0]->bytes();
        }
        
        void src(unsigned int n) {
            if(n<4) {
                MTLUtils::replace(this->_texture[HYDRA_OUT+n],this->_buffer[HYDRA_OUT+n]->bytes(),this->_buffer[HYDRA_OUT+n]->width(),this->_buffer[HYDRA_OUT+n]->height(),this->_buffer[HYDRA_OUT+n]->rowBytes());
            }
        }
            
        unsigned int *bytes(int n=0) {
            return (unsigned int *)this->_buffer[n]->bytes();
        }
    
        void set(unsigned int index, float value) {
            if(index<_arguments.size()-HYDRA_OFFSET_UNIFORM) {
                ((float *)[this->_arguments[HYDRA_OFFSET_UNIFORM+index] contents])[0] = value;
            }
        }
    
        void set(NSString *key, float value) {
            
            for(int k=0; k<[this->_uniformsKey count]; k++) {
                if([key compare:this->_uniformsKey[k]]==NSOrderedSame) {
                    if(this->_uniformsType[k]==Hydra::UniformType::FloatType) {
                        this->set(k,value);
                    }
                    break;
                }
            }
        }
    
        bool reloadShader(dispatch_data_t metallib, NSDictionary *json) {
            if(json) {
                
                [this->_uniforms removeAllObjects];
                this->_uniformsKey = [NSMutableArray array];
                
                for(NSString *key in [json[@"uniforms"] keyEnumerator]) {
                    [this->_uniforms setObject:json[@"uniforms"][key] forKey:key];
                    [this->_uniformsKey addObject:key];
                }
                
                this->sortUniformsKey();
                     
                if(metallib) {
                
                    NSError *error = nil;
                    this->_library = [this->_device newLibraryWithData:metallib error:&error];
                    
                    if(error==nil&&this->_library) {
                        if(this->setupPipelineState()) {
                            if(this->_useArgumentEncoder) {
                                this->setupArgumentEncoderWithBuffer();
                            }
                            return true;
                        }
                    }
                }
            }
            return false;
        }
            
        HydraComputeShader(int w,int h, dispatch_data_t metallib, NSDictionary *json) : ComputeShaderBase(w,h) {
        
            if(Hydra::jscontext==nil) {
                Hydra::jscontext = [JSContext new];
            }

            this->_useArgumentEncoder = true;

            this->_buffer.push_back(new MTLReadPixels<unsigned int>(w,h));
            
            MTLTextureDescriptor *RGBA8Unorm = MTLUtils::descriptor(MTLPixelFormatRGBA8Unorm,w,h);
            RGBA8Unorm.usage = MTLTextureUsageShaderWrite|MTLTextureUsageShaderRead;

            if(metallib&&json) {
                for(int k=0; k<(HYDRA_OUT+HYDRA_IN); k++) {
                    this->_texture.push_back([this->_device newTextureWithDescriptor:RGBA8Unorm]);
                }
                if(this->reloadShader(metallib,json)) {
                    this->_init = true;
                }
            }
        }
    
        HydraComputeShader(int w,int h,NSString *filename=@"default.metallib", NSString *identifier=nil) : ComputeShaderBase(w,h) {
            
            if(Hydra::jscontext==nil) {
                Hydra::jscontext = [JSContext new];
            }

            this->_useArgumentEncoder = true;

            this->_buffer.push_back(new MTLReadPixels<unsigned int>(w,h));
                        
            NSDictionary *json = nil;
            NSString *metallib = nil;
            
            if(FileManager::extension(filename,@"metallib")) {
                metallib = filename;
            }
            else if(FileManager::extension(filename,@"json")) {
                
                NSString *path = FileManager::path(filename,identifier);

                if(FileManager::exists(path)) {
                    NSString *str = [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
                    if(str) {
                        json = [NSJSONSerialization JSONObjectWithData:[str dataUsingEncoding:NSUnicodeStringEncoding] options:NSJSONReadingAllowFragments error:nil];
                        if(json) {
                                                        
                            if(json[@"metallib"]) {
                                metallib = json[@"metallib"];
                            }
                            
                            [this->_uniforms removeAllObjects];
                            this->_uniformsKey = [NSMutableArray array];

                            for(NSString *key in [json[@"uniforms"] keyEnumerator]) {
                                [this->_uniforms setObject:json[@"uniforms"][key] forKey:key];
                                [this->_uniformsKey addObject:key];
                            }
                                                        
                            this->sortUniformsKey();
                            
                        }
                    }
                }
            }
            else {
                NSLog(@"%@",[filename pathExtension]);
            }
                        
            if(metallib) {
            
                MTLTextureDescriptor *RGBA8Unorm = MTLUtils::descriptor(MTLPixelFormatRGBA8Unorm,w,h);
                RGBA8Unorm.usage = MTLTextureUsageShaderWrite|MTLTextureUsageShaderRead;

                for(int k=0; k<(HYDRA_OUT+HYDRA_IN); k++) {
                    this->_texture.push_back([this->_device newTextureWithDescriptor:RGBA8Unorm]);
                }
                
                if(ComputeShaderBase::setup(metallib,identifier)&&this->_useArgumentEncoder) {
                    this->setupArgumentEncoderWithBuffer();
                }
            }
        }
    
        ~HydraComputeShader() {
            [this->_uniforms removeAllObjects];
            this->_uniforms = nil;
            this->_uniformsKey = nil;
            this->_uniformsValue = nil;
        }
};


