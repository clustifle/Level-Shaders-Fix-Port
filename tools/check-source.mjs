// Source audit for the recovery package, not a gameplay or performance test.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const source=read('src/main.cpp');
const manifest=JSON.parse(read('mod.json'));
assert.equal(manifest.geode,'5.11.2');
assert.equal(manifest.version,'v0.1.0-alpha.2');
assert.equal(manifest.id,'vuthinh.level-shaders-fix-port');
assert.equal(manifest.gd.win,manifest.gd.android);
assert.equal(manifest.gd.win,'2.2081');
assert(manifest.incompatibilities['cgytrus.level-shaders-fix'].breaking);
assert(!manifest.resources,'Recovery must not package the obsolete custom shaders');
assert(!source.includes('$modify'),'Recovery must not install detours');
for(const name of ['ShaderLayer','CCRenderTexture','glIsProgram','glGetIntegerv',
                   'm_fContentScaleFactor','setAnimationInterval','setSwapInterval']) {
 assert(!source.includes(name),'Recovery has a rendering/frame-control dependency: '+name);
}
assert(source.includes('$execute'));
assert(source.includes('getGameVersion()'));
console.log('PASS recovery: no render detours, no custom shader resources, no per-frame GL or FPS controls');
console.log('NOT TESTED: runtime visual output and FPS after replacing alpha.1');
