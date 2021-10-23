template <typename T>
class ComputeShaderBase {
    
    private:
        
        bool _init = false;
    
    protected:

        id<MTLDevice> _device = MTLCreateSystemDefaultDevice();
        id<MTLFunction> _function = nil;
        id<MTLComputePipelineState> _pipelineState = nil;
        id<MTLLibrary> _library = nil;
        std::vector<id<MTLTexture>> _texture;
        std::vector<id<MTLBuffer>> _params;
        std::vector<T *> _buffer;

        bool _useArgumentEncoder = false;
        id<MTLArgumentEncoder> _argumentEncoder = nil;
        id<MTLBuffer> _argumentEncoderBuffer = nil;
        std::vector<id<MTLBuffer>> _arguments;

        int _width = 0;
        int _height = 0;

        ComputeShaderBase(int w, int h) {
            this->_width = w;
            this->_height = h;
        }
        
        ~ComputeShaderBase() {
            
            this->_device = nil;
            this->_function = nil;
            this->_pipelineState = nil;
            this->_library = nil;
            
            for(int k=0; k<this->_params.size(); k++) {
                this->_params[k] = nil;
            }
 
            for(int k=0; k<this->_texture.size(); k++) {
                this->_texture[k] = nil;
            }
            
            for(int k=0; k<this->_buffer.size(); k++) {
                delete[] this->_buffer[k];
            }
            
            for(int k=0; k<this->_arguments.size(); k++) {
                this->_arguments[k] = nil;
            }
        }
    
        MTLTextureDescriptor *descriptor(MTLPixelFormat format,int w,int h) {
            return [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];
        }
    
        bool init() { return this->_init; }
    
        void replace(id<MTLTexture> texture, T *data, int rowBytes) {
           [texture replaceRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0 withBytes:data bytesPerRow:rowBytes];
        }
 
        void copy(T *data, id<MTLTexture> texture, int rowBytes) {
            [texture getBytes:data bytesPerRow:rowBytes fromRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0];
        }
    
        void fill(T *data,T value, int rowBytes, int num) {
            for(int i=0; i<this->_height; i++) {
                for(int j=0; j<this->_width; j++) {
                    for(int n=0; n<num; n++) {
                        data[i*rowBytes+j] = value;
                    }
                }
            }
        }
    
        id<MTLBuffer> newBuffer(long length, MTLResourceOptions options = MTLResourceOptionCPUCacheModeDefault) {
            return [this->_device newBufferWithLength:length options:options];
        }
        
        bool setup(NSString *filename, NSString *func=@"processimage") {
            NSError *error = nil;
            this->_library = [this->_device newLibraryWithFile:filename error:&error];
            if(error==nil&&this->_library) {
                this->_function = [this->_library newFunctionWithName:func];
                this->_pipelineState = [this->_device newComputePipelineStateWithFunction:this->_function error:&error];
                if(error==nil) this->_init = true;
            }
            return this->_init;
        }
    
        void update() {
            
            if(this->_init) {
            
                id<MTLCommandQueue> queue = [this->_device newCommandQueue];

                id<MTLCommandBuffer> commandBuffer = queue.commandBuffer;
                id<MTLComputeCommandEncoder> encoder = commandBuffer.computeCommandEncoder;
                [encoder setComputePipelineState:this->_pipelineState];
                
                [encoder setTexture:this->_texture[0] atIndex:0];
                [encoder useResource:this->_texture[0] usage:MTLResourceUsageWrite];
                
                for(int k=1; k<this->_texture.size(); k++) {
                    [encoder setTexture:this->_texture[k] atIndex:k];
                    [encoder useResource:this->_texture[k] usage:MTLResourceUsageSample];
                }
                                      
                for(int k=0; k<this->_params.size(); k++) {
                    [encoder setBuffer:this->_params[k] offset:0 atIndex:k];
                }
                
                if(this->_useArgumentEncoder) [encoder setBuffer:this->_argumentEncoderBuffer offset:0 atIndex:0];
                
                int w = (int)this->_texture[0].width;
                int h = (int)this->_texture[0].height;
                
                int tx = 1;
                int ty = 1;
                
                for(int k=1; k<5; k++) {
                    if(w%(1<<k)==0) tx = 1<<k;
                    if(h%(1<<k)==0) ty = 1<<k;
                }
                
                MTLSize threadGroupSize = MTLSizeMake(tx,ty,1);
                MTLSize threadGroups = MTLSizeMake(w/tx,h/ty,1);
                
                [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
                [encoder endEncoding];
                [commandBuffer commit];
                [commandBuffer waitUntilCompleted];
            }
        }
};

