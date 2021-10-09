const BUILD = true;
const FILENAME = "./hydra";
const OS = "macosx"; // macosx, iphoneos, iphonesimulator
const VERSION = 3.0;

slider=(value,min,max)=>"slider("+value+","+min+","+max+")";

function stringifyWithFunctions(object) {
	return JSON.stringify(object,(k,v) => {
		if(typeof(v)==="function") return `(${v})`;
		return v;
	});
};

global["o0"] = {
	name:"args.o0",
	uniforms:{},
	getTexture:function() {},
	renderPasses:function(glsl) {			
        require("fs").writeFileSync(FILENAME+".json",'{\n\t"version":'+VERSION.toFixed(1)+',\n\t"metallib":"./hydra.metallib",\n\t"uniforms":'+stringifyWithFunctions(glsl[0].uniforms)+'\n}');
        require("fs").writeFileSync(FILENAME+".metal",glsl[0].frag);
		if(BUILD) {
			require("child_process").execSync("xcrun -sdk "+OS+" metal -c "+FILENAME+".metal -o "+FILENAME+".air; xcrun -sdk "+OS+" metallib "+FILENAME+".air -o "+FILENAME+".metallib");
		}
	}
};
	
for(let k=0; k<4; k++) {
	global["s"+k] = {
		name:"args.s"+k,
		uniforms:{},
		getTexture:function() {},
	};
}

const gen = new (require('./MSLGeneratorFactory.js'))(o0)
global.generator = gen
Object.keys(gen.functions).forEach((key)=>{
	global[key] = gen.functions[key];  
})

module.exports = {}
