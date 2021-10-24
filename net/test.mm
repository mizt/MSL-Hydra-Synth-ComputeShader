#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
namespace stb_image {
    #import "stb_image_write.h"
}

#import <vector>

#import "FileManager.h"
#import "MTLUtils.h"
#import "MTLReadPixels.h"
#import "PixelBuffer.h"
#import "ComputeShaderBase.h"
#import "HydraComputeShader.h"

namespace FileManager {

    NSString *removePlatform(NSString *str) {
        NSString *extension = [NSString stringWithFormat:@".%@",[str pathExtension]];
        return FileManager::replace(str,@[
            [NSString stringWithFormat:@"-macosx%@",extension],
            [NSString stringWithFormat:@"-iphoneos%@",extension],
            [NSString stringWithFormat:@"-iphonesimulator%@",extension]],
            extension);
    }

    NSString *addPlatform(NSString *str) {
        NSString *extension = [NSString stringWithFormat:@".%@",[str pathExtension]];
#if TARGET_OS_OSX
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-macosx%@",extension]);
#elif TARGET_OS_SIMULATOR
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-iphonesimulator%@",extension]);
#elif TARGET_OS_IPHONE
        return FileManager::replace(FileManager::removePlatform(str),extension,[NSString stringWithFormat:@"-iphoneos%@",extension]);
#else
        return nil;
#endif
    }

};

class App {
    
    private:
    
        bool _lock = false;
    
        NSString *baseURL = @"https://raw.githubusercontent.com/mizt/MSL-Hydra-Synth-ComputeShader/master";
        
        void load(NSString *filename, void (^callback)(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error)) {
            NSURL *json = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%@",baseURL,filename]];
            NSURLRequest *request = [NSURLRequest requestWithURL:json cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10];
            NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration];
            NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:callback];
            [task resume];
        }
        
    public:
        
        App() {
            
            this->load(@"hydra.json",^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error){
                if(response&&!error) {
                    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:nil];
                    if(json&&json[@"metallib"]) {
                        
                        NSString *filename = FileManager::addPlatform(json[@"metallib"]);
                        NSLog(@"%@",filename);
                        
                        if(filename) {
                            
                            this->_lock = true;

                            this->load(filename,^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                if(response&&!error) {
                                    
                                    @autoreleasepool {
                                                                             
                                        dispatch_data_t metallib = dispatch_data_create(data.bytes,data.length,DISPATCH_TARGET_QUEUE_DEFAULT,DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                                        
                                        if(metallib) {
                                            
                                            int w = 1920;
                                            int h = 1080;
                                            
                                            HydraComputeShader *hydra = new HydraComputeShader(w,h,metallib,json);
                                                    
                                            stb_image::stbi_write_png("test.png",w,h,4,(void const *)hydra->exec(),w<<2);
                                            
                                            delete hydra;
                                            
                                        }
                                    }
                                }
                                
                                this->_lock = false;
                                
                            });
                        }
                    }
                }
            });
        }
        
        ~App() {
            
        }
};

@interface AppDelegate:NSObject <NSApplicationDelegate> {
    App *m;
}
@end

@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    m = new App();
}
-(void)applicationWillTerminate:(NSNotification *)aNotification {
    if(m) delete m;
}
@end

int main (int argc, const char * argv[]) {
    
    id app = [NSApplication sharedApplication];
    id delegat = [AppDelegate alloc];
    [app setDelegate:delegat];
    [app run];
    return 0;
    
}
