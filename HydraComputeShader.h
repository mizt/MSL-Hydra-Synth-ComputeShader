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
    
        std::vector<Hydra::UniformType> _uniformsType;
        NSMutableArray *_uniformsKey = [NSMutableArray array];
        NSMutableArray *_uniformsValue = [NSMutableArray array];
        
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

                this->set(this->_arguments[HYDRA_UNIFORM_TIME],CFAbsoluteTimeGetCurrent()-startTime);

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
        
        HydraComputeShader(int w,int h,NSString *filename=@"default.metallib", NSString *identifier=nil) : ComputeShaderBase(w,h) {
            
            if(Hydra::jscontext==nil) {
                Hydra::jscontext = [JSContext new];
            }

            this->_useArgumentEncoder = true;

            this->_buffer.push_back(new MTLReadPixels<unsigned int>(w,h));
                        
            NSDictionary *json = nil;
            NSString *metallib = nil;
            
            if(FileManager::extension(filename,@"metallib")) {
                metallib = filename;//FileManager::path(filename,identifier);
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
                            
                            for(id key in [json[@"uniforms"] keyEnumerator]) {
                                [this->_uniformsKey addObject:key];
                            }
                            
                            this->_uniformsKey = (NSMutableArray *)[(NSArray *)this->_uniformsKey sortedArrayUsingComparator:^NSComparisonResult(NSString *s1,NSString *s2) {
                                int n1 = [[s1 componentsSeparatedByString:@"_"][1] intValue];
                                int n2 = [[s2 componentsSeparatedByString:@"_"][1] intValue];
                                if(n1<n2) return (NSComparisonResult)NSOrderedAscending;
                                else if(n1>n2) return (NSComparisonResult)NSOrderedDescending;
                                else return (NSComparisonResult)NSOrderedSame;
                            }];
                            
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
                    
                    this->_argumentEncoder = [this->_function newArgumentEncoderWithBufferIndex:0];
                    this->_argumentEncoderBuffer = MTLUtils::newBuffer(this->_device,sizeof(float)*[this->_argumentEncoder encodedLength]);
                    [this->_argumentEncoder setArgumentBuffer:this->_argumentEncoderBuffer offset:0];
                    
                    
                    this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)));
                    this->set(this->_arguments[HYDRA_UNIFORM_TIME],0);
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_TIME] offset:0 atIndex:0];
                    
                    this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)*2));
                    this->set(this->_arguments[HYDRA_UNIFORM_RESOLUTION],w,h);
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_RESOLUTION] offset:0 atIndex:1];

                    this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)*2));
                    this->set(this->_arguments[HYDRA_UNIFORM_MOUSE],0,0);
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_MOUSE] offset:0 atIndex:2];
                    
                    for(int k=0; k<[this->_uniformsKey count]; k++) {
                        
                        if([this->_uniformsValue count]<=k) { // add
                            
                            [this->_uniformsValue addObject:json[@"uniforms"][this->_uniformsKey[k]]];
                            this->_arguments.push_back(MTLUtils::newBuffer(this->_device,sizeof(float)));
                            [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_OFFSET_UNIFORM+k] offset:0 atIndex:HYDRA_OFFSET_UNIFORM+k];

                        }
                        else { // overwrite
                            this->_uniformsValue[k] = json[@"uniforms"][this->_uniformsKey[k]];
                        }
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
                else {
                    this->_argumentEncoder = nil;
                }
            }
        }
    
        ~HydraComputeShader() {
            this->_uniformsKey = nil;
            this->_uniformsValue = nil;
        }
};


