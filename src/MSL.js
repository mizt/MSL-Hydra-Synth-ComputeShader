const BUILD = true;
const FILENAME = "hydra";
const OS = ["macosx","iphoneos","iphonesimulator"];
const VERSION = 3.0;

slider=(value,min,max)=>"slider("+value+","+min+","+max+")";

const stringifyWithFunctions = function(object) {
	return JSON.stringify(object,(k,v) => {
		if(typeof(v)==="function") return "(${v})";
		return v;
	});
};

global["o0"] = {
	name:"o0",
	uniforms:{},
	getTexture:function() {},
	renderPasses:function(glsl) {			
        require("fs").writeFileSync(FILENAME+".json","{\n\t\"version\":"+VERSION.toFixed(1)+",\n\t\"metallib\":"+FILENAME+".metallib\",\n\t\"uniforms\":"+stringifyWithFunctions(glsl[0].uniforms)+"\n}");
        require("fs").writeFileSync(FILENAME+".metal",glsl[0].frag);
		if(BUILD) {
            for(let k=0; k<OS.length; k++) {
                require("child_process").execSync("xcrun -sdk "+OS[k]+" metal -c ./"+FILENAME+".metal -o ./"+FILENAME+"-"+OS[k]+".air; xcrun -sdk "+OS[k]+" metallib ./"+FILENAME+"-"+OS[k]+".air -o ./"+FILENAME+"-"+OS[k]+".metallib");
            }
            
            require("child_process").execSync("rm ./*.air");
		}
	}
};

for(let k=0; k<4; k++) {
	global["s"+k] = {
		name:"s"+k,
		uniforms:{},
		getTexture:function(){},
	};
}

const gen = new (require('./MSLGeneratorFactory.js'))(o0)
global.generator = gen
Object.keys(gen.functions).forEach((key)=>{
	global[key] = gen.functions[key];  
})

module.exports = {}
