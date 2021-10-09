### MSL-Hydra-Synth-ComputeShader

Please see more about hydra-synth.

 [ojack](https://github.com/ojack) / [hydra-synth](https://github.com/ojack/hydra-synth) 

##### Note:

The number of output buffers that can be used is 1.

### Build Command Line Tool & Run

```
$ node main.js
$ xcrun clang++ -ObjC++ -lc++ -fobjc-arc -O3 -std=c++17 -Wc++17-extensions -framework Cocoa -framework Metal -framework JavascriptCore ./test.mm -o ./test
$ ./test
```

