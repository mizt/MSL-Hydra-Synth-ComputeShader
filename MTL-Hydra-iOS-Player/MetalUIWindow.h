
@interface MetalViewController:UIViewController @end
@implementation MetalViewController
-(BOOL)prefersStatusBarHidden { return YES; }
@end

class MetalUIWindow {
    
    private:
        
        UIWindow *_window = nil;
        UIViewController *_controller = nil;
        float _scaleFactor;
    
        CGRect _bounds = [[UIScreen mainScreen] bounds];
    
        MetalUIWindow() {
            
            this->_scaleFactor = [UIScreen mainScreen].scale;
            this->_window = [[UIWindow alloc] initWithFrame:this->_bounds];
            this->_controller = [[MetalViewController alloc] init];
            this->_controller.view = [[UIView alloc] initWithFrame:this->_bounds];
            [this->_window setRootViewController:this->_controller];
            
        }
    
        ~MetalUIWindow() = default;

    public:
    
        UIWindow *window() {
            return this->_window;
        }
    
        UIView *view() {
            return this->_controller.view;
        }
    
        CGRect bounds() {
            return  this->_bounds;
        }
    
        float scaleFactor() {
            return this->_scaleFactor;
        }
    
        void appear() {
            [this->_window makeKeyAndVisible];
        }
        
        static MetalUIWindow *$() {
            static MetalUIWindow *instance = nullptr;
            if(instance==nullptr) {
                instance = new MetalUIWindow();
            }
            return instance;
        }
};
