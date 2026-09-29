// Uses NODEFS only for the command-line test; browser input uses WORKERFS.
import assert from 'node:assert/strict';
import path from 'node:path';
import {createHash} from 'node:crypto';
import createMelee from '../build/web/melee.mjs';
const disc = path.resolve(process.argv[2] || 'native/build/simulator-test.ciso');
const m = await createMelee();
m.FS.mkdir('/disc');
m.FS.mount(m.NODEFS, {root:path.dirname(disc)}, '/disc');
const check = value => assert.ok(value, m.UTF8ToString(m._web_error()));
check(m.ccall('web_open_disc', 'number', ['string'], ['/disc/'+path.basename(disc)]));
const report=[];
for (const fighter of [0,1,2,0]) {
  check(m._web_load_fighter(fighter));
  const frames=[];
  for (const frame of [0,15,30,60]) {
    check(m._web_step(frame));
    const hash=createHash('sha256');let vertices=0,indices=0,textures=0,visible=0;
    for(let i=0;i<m._web_part_count();i++) {
      const p=m._web_part_info(i);const info=new Uint32Array(m.HEAPU8.buffer,p,32).slice();
      const v=new Float32Array(m.HEAPU8.buffer,info[0],info[1]*12);
      assert.ok(v.every(Number.isFinite));
      const ix=new Uint32Array(m.HEAPU8.buffer,info[2],info[3]);
      assert.ok(ix.every(x=>x<info[1]));
      hash.update(new Uint8Array(v.buffer,v.byteOffset,v.byteLength));
      vertices+=info[1];indices+=info[3];textures+=!!info[4];visible+=!info[9];
    }
    assert.ok(vertices>0 && indices>0 && visible>0 && textures>0);
    frames.push({frame,vertices,indices,textures,visible,hash:hash.digest('hex')});
  }
  assert.notEqual(frames[0].hash,frames[1].hash,'Animation must change geometry');
  report.push({fighter,parts:m._web_part_count(),duration:m._web_duration(),omittedLayers:m._web_omitted_layers(),frames});
}
m._web_close_scene();
assert.equal(m._web_part_count(),0);
assert.equal(m._web_load_fighter(999),0);
console.log(JSON.stringify(report,null,2));
