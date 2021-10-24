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

class App {
    
    private:
    
        NSString *baseURL = @"https://raw.githubusercontent.com/mizt/MSL-Hydra-Synth-ComputeShader/master";
        
        void on(NSString *filename, void (^callback)(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error)) {
            NSURL *json = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%@",baseURL,filename]];
            NSURLRequest *request = [NSURLRequest requestWithURL:json cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10];
            NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration];
            NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:callback];
            [task resume];
        }
        
    public:
        
        App() {
            
            on(@"hydra.json",^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error){
                if(response&&!error) {
                    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:nil];
                    if(json&&json[@"metallib"]) {
                        NSString *normalize = FileManager::replace(json[@"metallib"],@[@"-macosx.metallib",@"-iphoneos.metallib",@"-iphonesimulator.metallib"],@".metallib");
                        
                        NSString *macosx = FileManager::replace(normalize,@".metallib",@"-macosx.metallib");
                        NSLog(@"%@",macosx);
                        
                        on(macosx,^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error){
                            if(response&&!error) {
                                
                                @autoreleasepool {
                                                                         
                                    dispatch_data_t metallib = dispatch_data_create(data.bytes, data.length, DISPATCH_TARGET_QUEUE_DEFAULT, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                                    
                                    if(metallib) {
                                        
                                        int w = 1920;
                                        int h = 1080;
                                        
                                        HydraComputeShader *hydra = new HydraComputeShader(w,h,metallib,json);
                                                
                                        stb_image::stbi_write_png("test.png",w,h,4,(void const *)hydra->exec(),w<<2);
                                        
                                        delete hydra;
                                        
                                    }
                                }
                            }
                            else {                                
                            }
                        });
                    }
                    else {
                    }
                }
                else {
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
