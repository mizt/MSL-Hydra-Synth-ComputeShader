#import <Foundation/Foundation.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
namespace stb_image {
    #import "stb_image_write.h"
}

#import "HydraComputeShader.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        int w = 1920;
        int h = 1080;
        
        HydraComputeShader *hydra = new HydraComputeShader(w,h,@"hydra.json");
                
        stb_image::stbi_write_png("test.png",w,h,4,(void const *)hydra->exec(),w<<2);
        
        delete hydra;
    }
    return 0;
}
