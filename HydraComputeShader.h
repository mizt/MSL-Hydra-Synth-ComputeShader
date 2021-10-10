#import <Metal/Metal.h>
#import <JavascriptCore/JavascriptCore.h>
#import <vector>

#import "ComputeShaderBase.h"

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
    
        unsigned int *exec(int n=0) {
            
            if(this->init()) {
                
                this->replace(this->_texture[1],this->_buffer[0],this->_width*4);

                ((float *)[this->_arguments[HYDRA_UNIFORM_TIME] contents])[0] = CFAbsoluteTimeGetCurrent() - startTime;
                
                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"time=%f;",((float *)[this->_arguments[HYDRA_UNIFORM_TIME] contents])[0]]];
                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"resolution={x:%d,y:%d};",this->_width,this->_height]];
                float *mouseBuffer = (float *)[this->_arguments[HYDRA_UNIFORM_MOUSE] contents];
                [Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"mouse={x:%f,y:%f};",mouseBuffer[0],mouseBuffer[1]]];
                
                for(int k=0; k<this->_uniformsType.size(); k++) {
                    Hydra::UniformType type = this->_uniformsType[k];
                    if(type==Hydra::UniformType::FunctionType) {
                        ((float *)[(id<MTLBuffer>)this->_arguments[HYDRA_OFFSET_UNIFORM+k] contents])[0] = [[Hydra::jscontext evaluateScript:[NSString stringWithFormat:@"(%@)();",this->_uniformsValue[k]]] toDouble];
                    }
                }
                
                ComputeShaderBase::update();
                this->copy(this->_buffer[0],this->_texture[0],this->_width*4);
                
            }
            return this->_buffer[n];
        }
        
        void src(unsigned int n) {
            if(n<4) {
                this->replace(this->_texture[HYDRA_OUT+n],this->_buffer[HYDRA_OUT+n],this->_width*4);
            }
        }
            
        unsigned int *bytes(int n=0) {
            return this->_buffer[n];
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

            this->_buffer.push_back(new unsigned int[w*h]);
            this->fill(this->_buffer[0],0x0,this->_width*4,0);
                        
            NSDictionary *json = nil;
            
            NSString *path = nil;
            if([[filename pathExtension] compare:@"json"]==NSOrderedSame) {
                json = [NSJSONSerialization JSONObjectWithData:[[[NSString alloc] initWithContentsOfFile:this->path(filename,identifier) encoding:NSUTF8StringEncoding error:nil] dataUsingEncoding:NSUnicodeStringEncoding] options:NSJSONReadingAllowFragments error:nil];
                
                if(json[@"metallib"]) {
                    path = this->path(json[@"metallib"],identifier);
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
            else if([[filename pathExtension] compare:@"metallib"]==NSOrderedSame) {
                path = this->path(filename,identifier);
            }
            
            if(path) {
               
                MTLTextureDescriptor *RGBA8Unorm = ComputeShaderBase::descriptor(MTLPixelFormatRGBA8Unorm,w,h);
                RGBA8Unorm.usage = MTLTextureUsageShaderWrite|MTLTextureUsageShaderRead;

                for(int k=0; k<(HYDRA_OUT+HYDRA_IN); k++) {
                    this->_texture.push_back([this->_device newTextureWithDescriptor:RGBA8Unorm]);
                }
                
                this->_params.push_back(this->newBuffer(sizeof(float)*2));
                float *resolution = (float *)[this->_params[0] contents];
                resolution[0] = w;
                resolution[1] = h;
                
                ComputeShaderBase::setup(path);
                
                if(this->_useArgumentEncoder) {
                    
                    this->_argumentEncoder = [this->_function newArgumentEncoderWithBufferIndex:0];
                    this->_argumentEncoderBuffer = this->newBuffer(sizeof(float)*[this->_argumentEncoder encodedLength],MTLResourceOptionCPUCacheModeDefault);
                    [this->_argumentEncoder setArgumentBuffer:this->_argumentEncoderBuffer offset:0];
                    
                    this->_arguments.push_back(this->newBuffer(sizeof(float),MTLResourceOptionCPUCacheModeDefault));
                    float *timeBuffer = (float *)[this->_arguments[HYDRA_UNIFORM_TIME] contents];
                    timeBuffer[0] = 0;
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_TIME] offset:0 atIndex:0];
                    
                    this->_arguments.push_back(this->newBuffer(sizeof(float)*2,MTLResourceOptionCPUCacheModeDefault));
                    float *resolutionBuffer = (float *)[this->_arguments[HYDRA_UNIFORM_RESOLUTION] contents];
                    resolutionBuffer[0] = w;
                    resolutionBuffer[1] = h;
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_RESOLUTION] offset:0 atIndex:1];

                    this->_arguments.push_back(this->newBuffer(sizeof(float)*2,MTLResourceOptionCPUCacheModeDefault));
                    float *mouseBuffer = (float *)[this->_arguments[HYDRA_UNIFORM_MOUSE] contents];
                    mouseBuffer[0] = 0;
                    mouseBuffer[1] = 0;
                    [this->_argumentEncoder setBuffer:this->_arguments[HYDRA_UNIFORM_MOUSE] offset:0 atIndex:2];
                    
                    for(int k=0; k<[this->_uniformsKey count]; k++) {
                        
                        if([this->_uniformsValue count]<=k) { // add
                            
                            [this->_uniformsValue addObject:json[@"uniforms"][this->_uniformsKey[k]]];
                            this->_arguments.push_back(this->newBuffer(sizeof(float),MTLResourceOptionCPUCacheModeDefault));
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
                            ((float *)[(id<MTLBuffer>)this->_arguments[HYDRA_OFFSET_UNIFORM+k] contents])[0] = [this->_uniformsValue[k] floatValue];
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


