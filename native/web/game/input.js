addToLibrary({
  melee_browser_poll_input__proxy:'sync',
  melee_browser_poll_input__sig:'vp',
  melee_browser_poll_input: (output) => {
    if(!Module.meleeKeys){
      const keys=Module.meleeKeys=new Set();
      const bound=new Set(['KeyW','KeyA','KeyS','KeyD','KeyJ','KeyK','KeyU','KeyI','KeyO','KeyQ','KeyE','Enter',
        'ArrowLeft','ArrowRight','ArrowDown','ArrowUp','Numpad1','Numpad2','Numpad4','Numpad5','Numpad6','Numpad7','Numpad9','NumpadEnter']);
      document.addEventListener('keydown',e=>{if(bound.has(e.code)){e.preventDefault();keys.add(e.code);}});
      document.addEventListener('keyup',e=>{keys.delete(e.code);if(bound.has(e.code))e.preventDefault();});
      window.addEventListener('blur',()=>keys.clear());
      document.addEventListener('visibilitychange',()=>{if(document.hidden)keys.clear();});
    }
    const keys=Module.meleeKeys;
    const maps=[['KeyA','KeyD','KeyS','KeyW','KeyJ','KeyK','KeyU','KeyI','KeyO','KeyQ','KeyE','Enter'],
      ['ArrowLeft','ArrowRight','ArrowDown','ArrowUp','Numpad1','Numpad2','Numpad4','Numpad5','Numpad6','Numpad7','Numpad9','NumpadEnter']];
    const controllers=Array.from(navigator.getGamepads?.()||[]).filter(p=>p&&p.connected&&p.mapping==='standard');
    for(let port=0;port<2;port++){
      const map=maps[port];const down=i=>keys.has(map[i])?1:0;
      let buttons=0;let x=down(1)-down(0);let y=down(3)-down(2);let cx=0;let cy=0;
      let l=down(9);let r=down(10);
      const bits=[256,512,1024,2048,16,64,32,4096];
      bits.forEach((bit,i)=>{if(down(i+4))buttons|=bit;});
      const pad=controllers[port];
      if(pad){
        const pressed=i=>pad.buttons[i]?.pressed;
        const mapping=[[0,256],[1,512],[2,1024],[3,2048],[4,16],[5,16],[8,4096],[9,4096],[12,8],[13,4],[14,1],[15,2]];
        for(const [index,bit] of mapping)if(pressed(index))buttons|=bit;
        const axis=i=>{const value=pad.axes[i]||0;return Math.abs(value)<0.15?0:value;};
        if(Math.abs(axis(0))>Math.abs(x))x=axis(0);if(Math.abs(axis(1))>Math.abs(y))y=-axis(1);
        cx=axis(2);cy=-axis(3);l=Math.max(l,pad.buttons[6]?.value||0);r=Math.max(r,pad.buttons[7]?.value||0);
        if(l>0.9)buttons|=64;if(r>0.9)buttons|=32;
      }
      HEAPF32.set([buttons,x,y,cx,cy,l,r,1],(output>>2)+port*8);
    }
  },
});
