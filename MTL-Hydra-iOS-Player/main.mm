#import <TargetConditionals.h>

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>

#import <JavascriptCore/JavascriptCore.h>

#import <vector>

#import "FileManager.h"
#import "MTLUtils.h"
#import "MTLReadPixels.h"
#import "PixelBuffer.h"

#import "Plane.h"
#import "MetalLayer.h"
#import "MetalUIWindow.h"

#import "ComputeShaderBase.h"
#import "HydraComputeShader.h"

class App {
    
    private:
        
        MetalView *_view = nil;
        MetalLayer<Plane> *_layer;
        HydraComputeShader *_hydra = nullptr;
        dispatch_source_t _timer;
        
    public:
    
        App() {
            CGRect bounds = MetalUIWindow::$()->bounds();
            float scaleFactor = MetalUIWindow::$()->scaleFactor();
            CGRect rect = CGRectMake(0,0,bounds.size.width*scaleFactor,bounds.size.height*scaleFactor);
            int w = rect.size.width;
            int h = rect.size.height;
            this->_hydra = new HydraComputeShader(w,h,@"hydra.json");
            this->_view = [[MetalView alloc] initWithFrame:CGRectMake(0,0,w,h)];
            this->_view.backgroundColor = [UIColor grayColor];
            this->_layer = new MetalLayer<Plane>((CAMetalLayer *)this->_view.layer);
            if(this->_layer->init(rect.size.width,rect.size.height,@"default.metallib")) {
                this->_view.frame = bounds;
                this->_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("ENTER_FRAME",0));
                dispatch_source_set_timer(this->_timer,dispatch_time(0,0),(1.0/60.0)*1000000000,0);
                dispatch_source_set_event_handler(this->_timer,^{
                    @autoreleasepool {
                        [this->_layer->texture() replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:this->_hydra->exec() bytesPerRow:w<<2];
                        this->_layer->update(^(id<MTLCommandBuffer> commandBuffer){
                            this->_layer->cleanup();
                            static dispatch_once_t oncePredicate;
                            dispatch_once(&oncePredicate,^{
                                dispatch_async(dispatch_get_main_queue(),^{
                                    [MetalUIWindow::$()->view() addSubview:this->_view];
                                    MetalUIWindow::$()->appear();
                                });
                            });
                        });
                    }
                });
                if(this->_timer) dispatch_resume(this->_timer);
            }
        }
    
        ~App() {
            if(this->_timer){
                dispatch_source_cancel(this->_timer);
                this->_timer = nullptr;
            }
            delete this->_hydra;
        }
};

@interface AppDelegate:UIResponder<UIApplicationDelegate> {
    App *app;
}
@end

@implementation AppDelegate

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    app = new App();
    return YES;
}

-(void)applicationWillTerminate:(UIApplication *)application {
    if(app) delete app;
}

@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc,argv,nil,@"AppDelegate");
    }
}
