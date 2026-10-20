// Stage a version-specific build without mutating the source manifest.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const gd = process.argv[2] ?? '2.2081';
if (!['2.2081', '2.209', '2.2091'].includes(gd)) throw new Error('Unsupported GD profile: ' + gd);
const dest = path.join(root, '.staging', gd);
// Copy a fixed list. Never recursively copy .staging into itself.
fs.mkdirSync(dest, {recursive:true});
for (const name of ['CMakeLists.txt', 'src', 'resources', 'about.md']) {
    fs.cpSync(path.join(root, name), path.join(dest, name), {recursive:true, force:true});
}
const manifest = JSON.parse(fs.readFileSync(path.join(root, 'mod.json'), 'utf8'));
manifest.gd = {win:gd, android:gd};
fs.writeFileSync(path.join(dest, 'mod.json'), JSON.stringify(manifest,null,4) + '\n');
console.log(dest);
