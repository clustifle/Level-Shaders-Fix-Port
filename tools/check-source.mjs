// A source/bindings audit, NOT a C++ compile or ABI/runtime test.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const source=read('src/main.cpp');
const manifest=JSON.parse(read('mod.json'));
const provenance=JSON.parse(read('PROVENANCE.json'));
const hookNames=['init','visit','prePixelateShader'];
assert.equal(manifest.geode,'5.11.2');
assert.equal(manifest.id,'vuthinh.level-shaders-fix-port');
assert.equal(manifest.gd.win,manifest.gd.android);
assert(manifest.incompatibilities['cgytrus.level-shaders-fix'].breaking);
assert(!/CCPoint\s+prepareTargetContainer\s*\(/.test(source),'Unavailable hook must not return');
assert(!/void\s+setupShader\s*\(/.test(source),'2.209 Windows setupShader address is absent');
assert(!/m_unknownE9[08]/.test(source),'Obsolete game fields');
assert(source.includes('getGameVersion() != std::string(LSF_TARGET_GD)'));
assert(!/base::get\(\)\s*\+\s*0x/.test(source),'No guessed addresses');
const fields=[...new Set(source.match(/\bm_[A-Za-z0-9_]+\b/g))].filter(x=>
    !['m_fields','m_fContentScaleFactor'].includes(x));
for(const gd of provenance.bindings.profiles){
 const bro=read('references/'+gd+'.bro');
 for(const field of fields) assert(bro.includes(field),gd+': unknown field '+field);
 const shader=bro.slice(bro.indexOf('class ShaderLayer'),bro.indexOf('\n}',bro.indexOf('class ShaderLayer'))+2);
 for(const hook of hookNames){
   const line=shader.split('\n').find(x=>new RegExp('\\b'+hook+'\\(').test(x));
   assert(line,gd+': missing '+hook);
   assert(/win 0x[0-9a-f]+/.test(line),gd+': missing Windows address '+hook);
   assert(!/win 0x9999999/.test(line),gd+': placeholder address '+hook);
 }
 for(const owner of ['m_objectParent','m_inShaderParent'])assert(bro.includes(owner));
 console.log('PASS '+gd+': 3 Windows hook addresses and '+fields.length+' field names present');
}
const fragment=read('resources/shaders/pp-frag.glsl');
const vertex=read('resources/shaders/pp-vert.glsl');
const bound=[...source.matchAll(/(m_[A-Za-z0-9_]+Uniform)\s*=\s*m_shader->getUniformLocationForName\("([^"]+)"\)/g)];
for(const [_,field,uniform] of bound){
 assert(provenance.bindings.profiles.every(gd=>read('references/'+gd+'.bro').includes(field)));
}
// These custom uniforms are essential to the rectangular target.
for(const uniform of ['_robHack','_camRot'])assert(fragment.includes('uniform vec2 '+uniform+';'));
assert(vertex.includes('CC_MVPMatrix'));
assert(fragment.includes('uniform vec4 _invertColorValue;'));
console.log('PASS metadata, version guard, custom shader uniforms and resource declarations');
console.log('NOT TESTED: compilation, binary ABI, GL driver behavior or in-game visuals/performance');
