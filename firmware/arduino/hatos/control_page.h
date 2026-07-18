#pragma once
// Self-contained control page served by the device over HTTP at http://192.168.4.1/
// Uses a browser WebSocket to ws://<host>:81 — no app install, works offline on the AP.
const char CONTROL_HTML[] = R"HTMLPAGE(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>HatOS Control</title>
<style>
:root{color-scheme:dark}
*{box-sizing:border-box;font-family:-apple-system,system-ui,sans-serif}
body{margin:0;background:#090c12;color:#f4f7fb;padding:16px;padding-bottom:40px}
h1{font-size:22px;margin:4px 0 2px}
.eyebrow{color:#8a94a8;font-size:11px;font-weight:800;letter-spacing:1.4px}
.card{background:#121720;border:1px solid #242c39;border-radius:16px;padding:14px;margin:10px 0;display:flex;flex-direction:column;gap:10px}
.row{display:flex;align-items:center;gap:8px;flex-wrap:wrap}
.between{justify-content:space-between}
.sec{font-size:15px;font-weight:800;margin-top:12px}
button{background:#151b25;border:1px solid #2b3441;border-radius:999px;color:#e9edf4;font-weight:700;font-size:14px;padding:10px 15px}
button:active{background:#232c3a}
.green{background:#b8ff65;color:#071008;border:none;border-radius:12px;font-weight:900;padding:12px 18px}
.on{background:#1d2a20;border-color:#78f294}
input{background:#05070a;border:1px solid #28303d;border-radius:10px;color:#f4f7fb;padding:10px 12px;font-size:14px;width:100%}
.dot{width:10px;height:10px;border-radius:5px;display:inline-block}
.hint{color:#727c90;font-size:12px}
#log{font-family:monospace;font-size:11px;color:#9aa4b5;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.step{width:38px;padding:8px 0;text-align:center}
</style></head>
<body>
<div class="eyebrow">HATOS / DEVICE CONTROL</div>
<h1>Hat Control</h1>
<div class="card"><div class="row between">
  <div class="row"><span class="dot" id="dot" style="background:#ff7a90"></span><b id="stat">connecting…</b></div>
  <button onclick="connect()">Reconnect</button>
</div></div>

<div class="sec">Active app</div>
<div class="row" id="apps"></div>

<div class="sec">Music · now playing</div>
<div class="card">
  <input id="title" value="Track Title" placeholder="title">
  <input id="artist" value="Artist Name" placeholder="artist">
  <div class="row between">
    <button id="play" class="on" onclick="togglePlay()">Playing</button>
    <div class="row"><button class="step" onclick="vol(-10)">–</button><b id="volv">VOL 60</b><button class="step" onclick="vol(10)">+</button></div>
  </div>
  <button class="green" onclick="pushMusic()">Push to hat</button>
</div>

<div class="sec">Maps · next maneuver</div>
<div class="card">
  <div class="row" id="turns"></div>
  <div class="row"><input id="dist" value="250" inputmode="numeric" style="flex:1" placeholder="distance m">
  <input id="street" value="Example Road" style="flex:2" placeholder="street"></div>
  <div class="hint">Tap a turn to push it with the distance + street above.</div>
</div>

<div class="sec">From hat</div>
<div class="card"><div id="log">encoder turns, presses, music.command appear here…</div></div>

<script>
var ws, playing=true, volume=60;
function set(t,c){document.getElementById('stat').textContent=t;document.getElementById('dot').style.background=c}
function connect(){
  try{ws&&ws.close()}catch(e){}
  set('connecting…','#ffce5c');
  ws=new WebSocket('ws://'+location.hostname+':81');
  ws.onopen=function(){set('connected','#78f294')};
  ws.onclose=function(){set('disconnected','#ff7a90');setTimeout(connect,1500)};
  ws.onerror=function(){set('error','#ff7a90')};
  ws.onmessage=function(e){var l=document.getElementById('log');l.textContent=e.data}
}
function send(o){try{ws&&ws.readyState==1&&ws.send(JSON.stringify(o))}catch(e){}}
['launcher','counter','notes','music','maps'].forEach(function(id){
  var b=document.createElement('button');b.textContent=id;
  b.onclick=function(){send({type:'app.setActive',payload:{id:id}})};
  document.getElementById('apps').appendChild(b)});
['left','slight-left','straight','slight-right','right','uturn'].forEach(function(t){
  var b=document.createElement('button');b.textContent=t;
  b.onclick=function(){send({type:'maps.maneuver',payload:{turn:t,distanceM:parseInt(document.getElementById('dist').value||'0',10),street:document.getElementById('street').value}})};
  document.getElementById('turns').appendChild(b)});
function togglePlay(){playing=!playing;var b=document.getElementById('play');b.textContent=playing?'Playing':'Paused';b.className=playing?'on':''}
function vol(d){volume=Math.max(0,Math.min(100,volume+d));document.getElementById('volv').textContent='VOL '+volume}
function pushMusic(){send({type:'music.state',payload:{title:document.getElementById('title').value,artist:document.getElementById('artist').value,positionMs:42000,durationMs:210000,isPlaying:playing,volume:volume}})}
connect();
</script></body></html>)HTMLPAGE";
