export function snapshot(m,full) {
  const parts=[], transfers=[];
  for(let i=0;i<m._web_part_count();i++) {
    const pointer=m._web_part_info(i);
    const info=new Uint32Array(m.HEAPU8.buffer,pointer,32).slice();
    const vertices=new Float32Array(m.HEAPU8.buffer,info[0],info[1]*12).slice();
    const part={vertices,hidden:!!info[9]};transfers.push(vertices.buffer);
    if(full) {
      part.indices=new Uint32Array(m.HEAPU8.buffer,info[2],info[3]).slice();
      part.width=info[4]?info[5]:1;part.height=info[4]?info[6]:1;
      part.pixels=info[4]?m.HEAPU8.slice(info[4],info[4]+part.width*part.height*4):new Uint8Array([255,255,255,255]);
      part.settings=new Float32Array(info.buffer,40,4).slice();
      part.pixel=Array.from(info.slice(14));part.wrap=Array.from(info.slice(7,9));
      transfers.push(part.indices.buffer,part.pixels.buffer);
    }
    parts.push(part);
  }
  return {parts,transfers};
}
