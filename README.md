### MSL-Hydra-Synth-ComputeShader

Based on  [hydra-synth 1.0.25 ](https://github.com/ojack/hydra-synth/tree/7eb0dde5175e2a6ce417e9f16d7e88fe1d750133).    
Please see more about  [ojack](https://github.com/ojack)/[hydra-synth](https://github.com/ojack/hydra-synth).

#### Note:

The number of output buffers that can be used is 1.

### Build Command Line Tool & Run

```
$ node main.js
$ xcrun clang++ -ObjC++ -lc++ -fobjc-arc -O3 -std=c++17 -Wc++17-extensions -framework Cocoa -framework Metal -framework JavascriptCore ./test.mm -o ./test
$ ./test
```

