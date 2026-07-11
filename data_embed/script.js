function val(id){return document.getElementById(id).value}
function set(id,v){document.getElementById(id).value=v}
function setSel(id,v){document.getElementById(id).value=v}
function msg(t,ok){var e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'msg-ok':'msg-err');e.style.display='block';setTimeout(function(){e.style.display='none'},4000)}

function loadConfig(){
  fetch('/api/config').then(r=>r.json()).then(c=>{
    set('call',c.call);set('fox',c.fox);
    setSel('mode',c.beaconMode?'beacon':'fox');
    setSel('fox_sync',c.foxSync?'on':'off');
    set('startup',c.startup);set('tx',c.tx);set('idle',c.idle);set('beacon_id',c.beaconId);
    set('wpm',c.wpm);set('tone',c.tone);
    setSel('warble',c.warble?'on':'off');
    set('warble_low',c.warbleLow);set('warble_high',c.warbleHigh);set('warble_step',c.warbleStep);
    setSel('ptt',c.pttActiveLow?'active_low':'active_high');
    set('lead',c.lead);set('tail',c.tail);
    setSel('battery',c.batteryEnabled?'on':'off');
    set('battery_scale',c.batteryScale);set('low_battery',c.lowBattery);
    setSel('wifi_ap',c.wifiAp?'on':'off');
    set('wifi_ap_timeout',c.wifiApTimeout);
    setSel('eco_mode',c.ecoMode?'on':'off');
    var s='<b>State:</b> <span>'+c.state+'</span> | <b>Battery:</b> <span>'+c.battery+' V</span>';
    if(c.foxSync&&!c.beaconMode&&c.foxNum>0)s+=' | <b>Slot:</b> <span>'+c.foxNum+'</span> (start '+c.startupResolved+'s)';
    s+='<br><b>AP:</b> <span>'+c.apSsid+'</span> | <b>IP:</b> <span>'+c.ip+'</span>';
    document.getElementById('status').innerHTML=s;
  }).catch(e=>msg('Load failed: '+e,false));
}

function save(){
  var d=new URLSearchParams();
  d.append('call',val('call'));d.append('fox',val('fox'));
  d.append('mode',val('mode'));d.append('fox_sync',val('fox_sync'));
  d.append('startup',val('startup'));d.append('tx',val('tx'));d.append('idle',val('idle'));
  d.append('beacon_id',val('beacon_id'));
  d.append('wpm',val('wpm'));d.append('tone',val('tone'));d.append('warble',val('warble'));
  d.append('warble_low',val('warble_low'));d.append('warble_high',val('warble_high'));d.append('warble_step',val('warble_step'));
  d.append('ptt',val('ptt'));d.append('lead',val('lead'));d.append('tail',val('tail'));
  d.append('battery',val('battery'));d.append('battery_scale',val('battery_scale'));d.append('low_battery',val('low_battery'));
  d.append('wifi_ap',val('wifi_ap'));d.append('wifi_ap_timeout',val('wifi_ap_timeout'));d.append('eco_mode',val('eco_mode'));
  fetch('/api/config',{method:'POST',body:d}).then(r=>r.text()).then(t=>{msg(t,true);setTimeout(loadConfig,500)}).catch(e=>msg('Save failed: '+e,false));
}

function cmd(c){
  fetch('/api/'+c,{method:'POST'}).then(r=>r.text()).then(t=>msg(t,true)).catch(e=>msg('Failed: '+e,false));
}

loadConfig();
