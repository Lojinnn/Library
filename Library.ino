#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>

// ==================== НАСТРОЙКИ ====================
const char* ssid     = "Loj";
const char* password = "LojinProject";

#define SS_USER   12      // считыватель A - карты читателей
#define RST_USER  22
#define SS_BOOK   21     // считыватель B - метки книг
#define RST_BOOK  13

#define TZ_SEC    (3 * 3600)   // часовой пояс (Москва +3)

MFRC522 rfidUser(SS_USER, RST_USER);
MFRC522 rfidBook(SS_BOOK, RST_BOOK);

String lastUserUID = "";
String lastBookUID = "";

WebServer server(80);

const char* F_USERS = "/users.csv";
const char* F_BOOKS = "/books.csv";
const char* F_HIST  = "/history.csv";

int histCounter = 0;

void pageHome(); void pageRegister(); void pageBooks(); void pageBook();
void pageReaders(); void pageReader(); void pageFormat();
void saveUser(); void saveBook();
void apiReader(); void apiBook(); void apiBorrow(); void apiReturn();

// ==================== СТИЛЬ ====================
const char CSS_FILE[] PROGMEM = R"CSS(
*{box-sizing:border-box;margin:0;padding:0}
:root{--ink:#0c0c0f;--paper:#fff;--bg:#f3f4f6;--line:#e4e5ea;--muted:#6b6e79;
--red:#e01b32;--red2:#b2152a;--rs:#fdeaec;--ok:#1b7a45;--oks:#e7f5ed;--wn:#8a5a12;--wns:#fbf0e0;
--rad:14px;--sh:0 1px 3px rgba(12,12,15,.05),0 6px 18px rgba(12,12,15,.05)}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Arial,sans-serif;background:var(--bg);color:var(--ink);font-size:15px;line-height:1.5}
.top{position:sticky;top:0;z-index:30;background:var(--ink);border-bottom:3px solid var(--red)}
.top-in{max-width:1200px;margin:0 auto;padding:11px 20px;display:flex;gap:16px;align-items:center;flex-wrap:wrap}
.brand{display:flex;gap:11px;align-items:center;color:#fff;text-decoration:none}
.mark{width:36px;height:36px;border-radius:10px;background:var(--red);color:#fff;display:flex;align-items:center;justify-content:center;font-weight:800;font-size:19px}
.brand b{display:block;font-size:15px;letter-spacing:3px}
.brand i{display:block;font-size:11px;color:#8d909c;letter-spacing:.6px;font-style:normal}
.nav{display:flex;gap:3px;flex-wrap:wrap;margin-left:auto}
.nav a{color:#b6b8c2;text-decoration:none;padding:8px 13px;border-radius:9px;font-size:13px;font-weight:600;white-space:nowrap}
.nav a:hover{background:#26262e;color:#fff}
.nav a.on{background:var(--red);color:#fff}
.wrap{max-width:1200px;margin:0 auto;padding:22px 20px 60px}
.phead{display:flex;justify-content:space-between;align-items:flex-end;gap:14px;flex-wrap:wrap;margin-bottom:16px}
.phead h2{font-size:23px;letter-spacing:-.2px}
.phead p{color:var(--muted);font-size:13.5px;margin-top:3px;max-width:660px}
.card{background:var(--paper);border:1px solid var(--line);border-radius:var(--rad);box-shadow:var(--sh);margin-bottom:18px;overflow:hidden}
.chd{padding:14px 18px;border-bottom:1px solid var(--line);display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap}
.chd h3{font-size:14px;letter-spacing:.6px;text-transform:uppercase}
.bar{width:4px;height:15px;background:var(--red);border-radius:2px;display:inline-block;margin-right:9px;vertical-align:-2px}
.cbd{padding:18px}
.grid{display:grid;gap:18px}
.g2{grid-template-columns:repeat(auto-fit,minmax(320px,1fr))}
.g5{grid-template-columns:repeat(auto-fit,minmax(170px,1fr))}
.greg{grid-template-columns:minmax(300px,370px) minmax(320px,1fr)}
@media(max-width:880px){.greg{grid-template-columns:1fr}}
label{display:block;font-size:11.5px;font-weight:800;color:var(--muted);letter-spacing:.5px;text-transform:uppercase;margin-bottom:5px}
.f{margin-bottom:13px}
input,select{width:100%;padding:10px 12px;border:1px solid var(--line);border-radius:10px;font-size:14.5px;background:#fbfbfc;font-family:inherit;color:var(--ink)}
input:focus,select:focus{outline:none;border-color:var(--red);background:#fff;box-shadow:0 0 0 3px var(--rs)}
input[readonly]{font-family:ui-monospace,Consolas,monospace;font-weight:700;color:var(--red);background:#fff}
.f2{display:grid;grid-template-columns:1fr 1fr;gap:0 14px}
@media(max-width:560px){.f2{grid-template-columns:1fr}}
.sep{margin:18px 0 13px;font-size:11.5px;font-weight:800;letter-spacing:1.2px;text-transform:uppercase;border-top:1px solid var(--line);padding-top:14px}
.btn{display:inline-flex;align-items:center;justify-content:center;border:1px solid transparent;padding:11px 18px;border-radius:10px;font-size:14px;font-weight:700;cursor:pointer;text-decoration:none;transition:.15s;font-family:inherit}
.btn:disabled{opacity:.45;cursor:not-allowed}
.b-red{background:var(--red);color:#fff}.b-red:hover:not(:disabled){background:var(--red2)}
.b-ink{background:var(--ink);color:#fff}.b-ink:hover:not(:disabled){background:#2a2a33}
.b-out{background:#fff;color:var(--ink);border-color:var(--line)}.b-out:hover:not(:disabled){border-color:var(--red);color:var(--red)}
.b-scan{background:#fff;color:var(--ink);border:1.5px solid var(--red)}.b-scan:hover:not(:disabled){background:var(--red);color:#fff}
.b-sm{padding:7px 12px;font-size:12.5px;border-radius:8px}
.b-full{width:100%}
.acts{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px}
.acts .btn{flex:1;min-width:170px}
.tbl{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:14px}
th{background:var(--ink);color:#fff;text-align:left;padding:11px 14px;font-size:11px;letter-spacing:.8px;text-transform:uppercase;white-space:nowrap}
td{padding:11px 14px;border-bottom:1px solid var(--line);vertical-align:top}
tbody tr:last-child td{border-bottom:none}
tbody tr:hover td{background:#fafafb}
.uid{font-family:ui-monospace,Consolas,monospace;font-weight:700;color:var(--red);font-size:13px;white-space:nowrap}
.ttl{font-weight:700}
.sub{display:block;color:var(--muted);font-size:12.5px;margin-top:2px}
.tag{display:inline-block;padding:3px 10px;border-radius:999px;font-size:11.5px;font-weight:800;white-space:nowrap}
.t-ok{background:var(--oks);color:var(--ok)}
.t-out{background:var(--rs);color:var(--red2)}
.t-late{background:var(--wns);color:var(--wn)}
.none{text-align:center;color:var(--muted);padding:34px 10px}
.note{padding:13px 16px;border-radius:12px;margin-bottom:16px;font-size:14px;font-weight:600;border:1px solid}
.n-ok{background:var(--oks);color:var(--ok);border-color:#bfe3cd}
.n-er{background:var(--rs);color:var(--red2);border-color:#f3c3c9}
.scan{border:1.5px dashed #cfd2d8;border-radius:12px;padding:16px;background:#fbfbfc;text-align:center}
.scan.on{border-color:var(--red);background:var(--rs);border-style:solid}
.scan .cap{font-size:11px;font-weight:800;letter-spacing:1.2px;color:var(--muted);text-transform:uppercase}
.scan .val{font-family:ui-monospace,Consolas,monospace;font-size:20px;font-weight:800;color:var(--red);margin-top:6px;word-break:break-all}
.step{display:flex;gap:12px;align-items:flex-start}
.num{flex:none;width:26px;height:26px;border-radius:50%;background:var(--ink);color:#fff;font-size:13px;font-weight:800;display:flex;align-items:center;justify-content:center;margin-top:2px}
.step.on .num{background:var(--red)}
.step .body{flex:1;min-width:0}
.info{margin-top:10px;font-size:13.5px;min-height:22px}
.panel{background:#fff;border:1px solid var(--line);border-radius:var(--rad);box-shadow:var(--sh);padding:14px 16px;margin-bottom:18px;display:flex;gap:12px;align-items:center;flex-wrap:wrap}
.panel .res{font-family:ui-monospace,Consolas,monospace;font-weight:800;color:var(--red)}
.panel select{width:auto;min-width:130px}
.chips{display:flex;gap:6px;flex-wrap:wrap}
.chip{border:1px solid var(--line);background:#fff;border-radius:999px;padding:6px 13px;font-size:12.5px;font-weight:700;cursor:pointer;color:var(--muted);font-family:inherit}
.chip.on{background:var(--ink);color:#fff;border-color:var(--ink)}
.search{position:relative;flex:1;min-width:240px}
.sug{display:none;position:absolute;left:0;right:0;top:calc(100% + 6px);background:#fff;border:1px solid var(--line);border-radius:12px;box-shadow:0 12px 30px rgba(12,12,15,.16);z-index:40;max-height:320px;overflow:auto}
.sug-i{padding:10px 14px;cursor:pointer;border-bottom:1px solid var(--line)}
.sug-i:last-child{border-bottom:none}
.sug-i:hover,.sug-i.on{background:var(--rs)}
.sug-t{display:block;font-weight:700;font-size:14px}
.sug-m{display:block;color:var(--muted);font-size:12.5px;margin-top:2px}
.sug-i b{color:var(--red)}
.sug-e{padding:12px 14px;color:var(--muted);font-size:13px}
.found{font-size:12.5px;color:var(--muted);font-weight:700;white-space:nowrap}
.dl{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px 22px}
.dl div{border-bottom:1px dashed var(--line);padding-bottom:8px}
.dl dt{font-size:11px;font-weight:800;letter-spacing:.8px;text-transform:uppercase;color:var(--muted)}
.dl dd{font-size:15px;margin-top:3px;word-break:break-word}
.stat{background:#fff;border:1px solid var(--line);border-radius:var(--rad);box-shadow:var(--sh);padding:16px 18px}
.stat b{display:block;font-size:30px;line-height:1.1;letter-spacing:-1px}
.stat span{display:block;font-size:11.5px;font-weight:800;letter-spacing:.7px;text-transform:uppercase;color:var(--muted);margin-top:4px}
.stat.red b{color:var(--red)}
.back{display:inline-block;margin-bottom:14px;color:var(--red);font-weight:700;text-decoration:none;font-size:14px}
.back:hover{text-decoration:underline}
.hit td{background:var(--rs)!important;box-shadow:inset 3px 0 0 var(--red)}
.mono{font-family:ui-monospace,Consolas,monospace}
.hint{color:var(--muted);font-size:12.5px;margin-top:8px}
.box{background:var(--rs);border:1px solid #f3c3c9;border-radius:12px;padding:14px 16px;margin-top:16px;font-size:14px}
.box b{color:var(--red2)}
)CSS";

// ==================== СКРИПТ ====================
const char JS_FILE[] PROGMEM = R"JS(
function el(i){return document.getElementById(i);}
function note(h){var m=el('msgbox');if(!m)return;m.innerHTML=h;window.scrollTo({top:0,behavior:'smooth'});setTimeout(function(){m.innerHTML='';},9000);}
function grab(u,cb){fetch(u,{cache:'no-store'}).then(function(r){return r.text();}).then(function(t){cb(null,t);}).catch(function(e){cb(e,null);});}
function busy(b,on){if(on){b.dataset.t=b.textContent;b.textContent='Поиск метки...';b.disabled=true;}else{if(b.dataset.t)b.textContent=b.dataset.t;b.disabled=false;}}
function noTag(){alert('Метка не обнаружена. Приложите метку к нужному считывателю и нажмите кнопку снова.');}

function scanTo(u,id,b,next){
  busy(b,1);
  grab(u,function(e,d){
    busy(b,0);
    if(e){alert('Ошибка связи: '+e);return;}
    d=(d||'').trim();
    if(!d){noTag();return;}
    el(id).value=d;
    if(next&&el(next))el(next).focus();
  });
}

var rUID='',bUID='';
function refresh(){var g=el('goBtn');if(g)g.disabled=!(rUID&&bUID);}

function scanStep(u,box,val,info,iu,b,kind){
  busy(b,1);
  grab(u,function(e,d){
    busy(b,0);
    if(e){alert('Ошибка связи: '+e);return;}
    d=(d||'').trim();
    if(!d){noTag();return;}
    if(kind==='r'){rUID=d;el('rStep').classList.add('on');}
    else{bUID=d;el('bStep').classList.add('on');}
    el(val).textContent=d;
    el(box).classList.add('on');
    el(info).innerHTML='загрузка данных...';
    grab(iu+'?uid='+encodeURIComponent(d),function(e2,t2){el(info).innerHTML=e2?'нет данных':t2;});
    refresh();
  });
}

function clearSteps(){
  rUID='';bUID='';
  el('rVal').innerHTML='&mdash;';el('bVal').innerHTML='&mdash;';
  el('rBox').classList.remove('on');el('bBox').classList.remove('on');
  el('rStep').classList.remove('on');el('bStep').classList.remove('on');
  el('rInfo').innerHTML='';el('bInfo').innerHTML='';
  refresh();
}

function doBorrow(){
  if(!rUID){alert('Сначала считайте карту читателя');return;}
  if(!bUID){alert('Сначала считайте метку книги');return;}
  el('goBtn').disabled=true;
  grab('/api/borrow?r='+rUID+'&b='+bUID+'&d='+el('days').value,function(e,t){
    if(e){alert('Ошибка связи: '+e);refresh();return;}
    note(t);clearSteps();
  });
}

function doReturn(){
  if(!rUID){alert('Сначала считайте карту читателя');return;}
  if(!bUID){alert('Сначала считайте метку книги');return;}
  el('goBtn').disabled=true;
  grab('/api/return?r='+rUID+'&b='+bUID,function(e,t){
    if(e){alert('Ошибка связи: '+e);refresh();return;}
    note(t);clearSteps();
  });
}

function updDue(){
  var s=el('days'),o=el('duePreview');if(!s||!o)return;
  var d=new Date(Date.now()+parseInt(s.value,10)*86400000);
  o.value=('0'+d.getDate()).slice(-2)+'.'+('0'+(d.getMonth()+1)).slice(-2)+'.'+d.getFullYear();
}

/* ---------- живой поиск по складу ---------- */
var bq='',bscope='all',bst='all',si=-1;

function brows(){return el('tblBooks').tBodies[0].rows;}

function rtext(r,sc){
  var t=r.getAttribute('data-t')||'',a=r.getAttribute('data-a')||'',y=r.getAttribute('data-y')||'';
  if(sc==='title')return t;
  if(sc==='author')return a;
  if(sc==='year')return y;
  return t+' '+a+' '+y;
}

function applyBooks(){
  var tb=el('tblBooks');if(!tb)return;
  var rs=tb.tBodies[0].rows,n=0,total=0,q=bq.toLowerCase().trim();
  for(var i=0;i<rs.length;i++){
    var r=rs[i];if(!r.getAttribute('data-uid'))continue;
    total++;
    var s=r.getAttribute('data-st');
    var okS=(bst==='all')||(s===bst)||(bst==='out'&&s==='late');
    var okQ=!q||rtext(r,bscope).toLowerCase().indexOf(q)>=0;
    var show=okS&&okQ;
    r.style.display=show?'':'none';
    if(show)n++;
  }
  var f=el('found');if(f)f.textContent=n;
  var nm=el('noMatch');if(nm)nm.style.display=(n===0&&total>0)?'':'none';
}

function mark(s,q){
  if(!q)return s;
  var i=s.toLowerCase().indexOf(q.toLowerCase());
  if(i<0)return s;
  return s.substring(0,i)+'<b>'+s.substring(i,i+q.length)+'</b>'+s.substring(i+q.length);
}

function hideSug(){var b=el('sug');if(b){b.style.display='none';b.innerHTML='';}si=-1;}

function suggest(){
  var box=el('sug');if(!box)return;
  var q=bq.trim();si=-1;
  if(q.length<1){hideSug();return;}
  var rs=brows(),out='',n=0;
  for(var i=0;i<rs.length&&n<8;i++){
    var r=rs[i];
    if(!r.getAttribute('data-uid'))continue;
    if(r.style.display==='none')continue;
    var t=r.getAttribute('data-t')||'',a=r.getAttribute('data-a')||'',y=r.getAttribute('data-y')||'';
    var uid=r.getAttribute('data-uid');
    var meta=mark(a,q);
    if(y)meta+=(meta?' &middot; ':'')+mark(y,q);
    out+='<div class="sug-i" onmousedown="pick(\''+uid+'\')">'
       +'<span class="sug-t">'+mark(t,q)+'</span>'
       +'<span class="sug-m">'+(meta||'&nbsp;')+'</span></div>';
    n++;
  }
  if(!n)out='<div class="sug-e">Совпадений нет</div>';
  box.innerHTML=out;box.style.display='block';
}

function pick(uid){
  var r=el('row_'+uid);hideSug();
  if(!r)return;
  var t=r.getAttribute('data-t')||'';
  var inp=el('q');if(inp)inp.value=t;
  bq=t;applyBooks();
  var rs=brows();for(var i=0;i<rs.length;i++)rs[i].classList.remove('hit');
  r.style.display='';r.classList.add('hit');
  r.scrollIntoView({behavior:'smooth',block:'center'});
}

function searchBooks(v){bq=v;applyBooks();suggest();}
function setScope(v){bscope=v;applyBooks();suggest();}

function setSt(st,b){
  bst=st;
  var c=b.parentNode.getElementsByClassName('chip');
  for(var j=0;j<c.length;j++)c[j].classList.remove('on');
  b.classList.add('on');
  applyBooks();suggest();
}

function sugKey(e){
  var box=el('sug');if(!box)return;
  var it=box.getElementsByClassName('sug-i');
  if(e.key==='ArrowDown'||e.key==='ArrowUp'){
    if(!it.length)return;
    e.preventDefault();
    si+=(e.key==='ArrowDown')?1:-1;
    if(si<0)si=it.length-1;
    if(si>=it.length)si=0;
    for(var i=0;i<it.length;i++){if(i===si)it[i].classList.add('on');else it[i].classList.remove('on');}
  }else if(e.key==='Enter'){
    if(it.length){e.preventDefault();var k=(si>=0&&si<it.length)?si:0;it[k].onmousedown();}
  }else if(e.key==='Escape'){hideSug();}
}

function scanFind(u,t,b){
  busy(b,1);
  grab(u,function(e,d){
    busy(b,0);
    var out=el('scanRes');
    if(e){out.textContent='ошибка связи';return;}
    d=(d||'').trim();
    if(!d){out.textContent='метка не обнаружена';return;}
    if(t==='tblBooks'){
      bq='';bst='all';bscope='all';
      var inp=el('q');if(inp)inp.value='';
      var sc=el('scope');if(sc)sc.value='all';
      var ch=document.querySelectorAll('.chips .chip');
      for(var i=0;i<ch.length;i++)ch[i].classList.remove('on');
      if(ch.length)ch[0].classList.add('on');
      applyBooks();hideSug();
    }
    var rs=el(t).tBodies[0].rows;
    for(var i=0;i<rs.length;i++)rs[i].classList.remove('hit');
    var row=el('row_'+d);
    if(row){out.textContent=d;row.style.display='';row.classList.add('hit');row.scrollIntoView({behavior:'smooth',block:'center'});}
    else out.textContent=d+' (нет в базе)';
  });
}

function filterRows(t,q){
  q=(q||'').toLowerCase().trim();
  var rs=el(t).tBodies[0].rows;
  for(var i=0;i<rs.length;i++){
    if(!rs[i].getAttribute('data-st'))continue;
    rs[i].style.display=(!q||rs[i].textContent.toLowerCase().indexOf(q)>=0)?'':'none';
  }
}

document.addEventListener('DOMContentLoaded',function(){
  if(el('tblBooks'))applyBooks();
  if(el('days'))updDue();
  refresh();
});
)JS";

// ==================== СТАТИЧЕСКИЕ СТРАНИЦЫ ====================
const char PG_BORROW[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="ru"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="stylesheet" href="/style.css"><title>Выдача</title></head><body>
<div class="top"><div class="top-in">
<a class="brand" href="/"><span class="mark">Б</span><span><b>БИБЛИОТЕКА</b><i>учёт фонда и читателей</i></span></a>
<div class="nav"><a href="/">Обзор</a><a href="/register">Регистрация</a><a href="/books">Склад</a><a href="/readers">Читатели</a><a href="/borrow" class="on">Выдача</a><a href="/return">Возврат</a></div>
</div></div><div class="wrap">
<div class="phead"><div><h2>Выдача книги</h2><p>Считыватель A — карты читателей, считыватель B — метки книг. Метка читается только по нажатию своей кнопки.</p></div><a class="btn b-out" href="/books">Открыть склад</a></div>
<div id="msgbox"></div>
<div class="card"><div class="chd"><h3><span class="bar"></span>Оформление выдачи</h3></div><div class="cbd">
<div class="grid g2">
<div class="step" id="rStep"><div class="num">1</div><div class="body">
<div class="scan" id="rBox"><div class="cap">Карта читателя</div><div class="val" id="rVal">&mdash;</div></div>
<button class="btn b-scan b-full" style="margin-top:12px" onclick="scanStep('/api/uid/user','rBox','rVal','rInfo','/api/reader',this,'r')">Считать карту читателя</button>
<div class="info" id="rInfo"></div></div></div>
<div class="step" id="bStep"><div class="num">2</div><div class="body">
<div class="scan" id="bBox"><div class="cap">Метка книги</div><div class="val" id="bVal">&mdash;</div></div>
<button class="btn b-scan b-full" style="margin-top:12px" onclick="scanStep('/api/uid/book','bBox','bVal','bInfo','/api/book',this,'b')">Считать метку книги</button>
<div class="info" id="bInfo"></div></div></div>
</div>
<div class="sep">Срок пользования</div>
<div class="f2">
<div class="f"><label>Выдать на</label><select id="days" onchange="updDue()">
<option value="7">7 дней</option><option value="14" selected>14 дней</option>
<option value="21">21 день</option><option value="30">30 дней</option>
<option value="45">45 дней</option><option value="60">60 дней</option></select></div>
<div class="f"><label>Вернуть до</label><input id="duePreview" readonly></div>
</div>
<div class="acts"><button class="btn b-red" id="goBtn" onclick="doBorrow()" disabled>Выдать книгу</button>
<button class="btn b-out" onclick="clearSteps()">Сбросить</button></div>
</div></div></div><script src="/app.js"></script></body></html>)HTML";

const char PG_RETURN[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="ru"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="stylesheet" href="/style.css"><title>Возврат</title></head><body>
<div class="top"><div class="top-in">
<a class="brand" href="/"><span class="mark">Б</span><span><b>БИБЛИОТЕКА</b><i>учёт фонда и читателей</i></span></a>
<div class="nav"><a href="/">Обзор</a><a href="/register">Регистрация</a><a href="/books">Склад</a><a href="/readers">Читатели</a><a href="/borrow">Выдача</a><a href="/return" class="on">Возврат</a></div>
</div></div><div class="wrap">
<div class="phead"><div><h2>Возврат книги</h2><p>Приёмка экземпляра обратно на склад. История операции сохранится у читателя и у книги.</p></div><a class="btn b-out" href="/readers">Читатели</a></div>
<div id="msgbox"></div>
<div class="card"><div class="chd"><h3><span class="bar"></span>Приёмка на склад</h3></div><div class="cbd">
<div class="grid g2">
<div class="step" id="rStep"><div class="num">1</div><div class="body">
<div class="scan" id="rBox"><div class="cap">Карта читателя</div><div class="val" id="rVal">&mdash;</div></div>
<button class="btn b-scan b-full" style="margin-top:12px" onclick="scanStep('/api/uid/user','rBox','rVal','rInfo','/api/reader',this,'r')">Считать карту читателя</button>
<div class="info" id="rInfo"></div></div></div>
<div class="step" id="bStep"><div class="num">2</div><div class="body">
<div class="scan" id="bBox"><div class="cap">Метка книги</div><div class="val" id="bVal">&mdash;</div></div>
<button class="btn b-scan b-full" style="margin-top:12px" onclick="scanStep('/api/uid/book','bBox','bVal','bInfo','/api/book',this,'b')">Считать метку книги</button>
<div class="info" id="bInfo"></div></div></div>
</div>
<div class="acts"><button class="btn b-red" id="goBtn" onclick="doReturn()" disabled>Принять на склад</button>
<button class="btn b-out" onclick="clearSteps()">Сбросить</button></div>
</div></div></div><script src="/app.js"></script></body></html>)HTML";

// ==================== КАРКАС СТРАНИЦ ====================
const char H_A[] PROGMEM = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><link rel=\"stylesheet\" href=\"/style.css\"><title>";
const char H_B[] PROGMEM = "</title></head><body><div class=\"top\"><div class=\"top-in\"><a class=\"brand\" href=\"/\"><span class=\"mark\">Б</span><span><b>БИБЛИОТЕКА</b><i>учёт фонда и читателей</i></span></a><div class=\"nav\">";
const char H_C[] PROGMEM = "</div></div></div><div class=\"wrap\">";
const char H_F[] PROGMEM = "</div><script src=\"/app.js\"></script></body></html>";

void navLink(const char* href, const char* text, bool on) {
  String s = "<a href=\""; s += href;
  s += on ? "\" class=\"on\">" : "\">";
  s += text; s += "</a>";
  server.sendContent(s);
}

void beginPage(const char* title, const char* active) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent_P(H_A);
  server.sendContent(title);
  server.sendContent_P(H_B);
  navLink("/",         "Обзор",       strcmp(active,"home")==0);
  navLink("/register", "Регистрация", strcmp(active,"reg")==0);
  navLink("/books",    "Склад",       strcmp(active,"books")==0);
  navLink("/readers",  "Читатели",    strcmp(active,"readers")==0);
  navLink("/borrow",   "Выдача",      strcmp(active,"borrow")==0);
  navLink("/return",   "Возврат",     strcmp(active,"return")==0);
  server.sendContent_P(H_C);
}

void endPage() { server.sendContent_P(H_F); server.sendContent(""); }

// ==================== УТИЛИТЫ ====================
String clean(String s, int maxLen = 100) {
  s.replace("\r"," "); s.replace("\n"," ");
  s.replace(";",",");  s.replace("\"","'");
  s.replace("<","(");  s.replace(">",")");
  s.trim();
  if ((int)s.length() > maxLen) s = s.substring(0, maxLen);
  return s;
}

String field(const String& line, int idx) {
  int st = 0;
  for (int i = 0; i < idx; i++) {
    int p = line.indexOf(';', st);
    if (p < 0) return "";
    st = p + 1;
  }
  int e = line.indexOf(';', st);
  if (e < 0) e = line.length();
  return line.substring(st, e);
}

int countFields(const String& s) {
  int n = 1;
  for (unsigned i = 0; i < s.length(); i++) if (s[i] == ';') n++;
  return n;
}

String findLine(const char* path, const String& uid) {
  File f = LittleFS.open(path, "r");
  if (!f) return "";
  String res = "";
  while (f.available()) {
    String ln = f.readStringUntil('\n'); ln.trim();
    if (!ln.length()) continue;
    if (field(ln,0) == uid) { res = ln; break; }
  }
  f.close();
  return res;
}

void replaceLine(const char* path, const String& uid, const String& nl) {
  String all = "";
  File f = LittleFS.open(path, "r");
  if (f) {
    // одна аллокация сразу под весь файл вместо сотен realloc
    if (!all.reserve(f.size() + 128)) {
      f.close();
      Serial.println("replaceLine: мало RAM, перезапись отменена");
      return;                      // файл остаётся целым, ничего не портится
    }
    while (f.available()) {
      String ln = f.readStringUntil('\n'); ln.trim();
      if (!ln.length()) continue;
      all += (field(ln,0) == uid) ? nl : ln;
      all += "\n";
    }
    f.close();
  }
  f = LittleFS.open(path, "w");
  if (f) { f.print(all); f.close(); }
}

unsigned long nowTs() {
  time_t t = time(nullptr);
  if (t < 1700000000) return 0;
  return (unsigned long)t;
}

String fmtDate(unsigned long ts) {
  if (!ts) return "&mdash;";
  time_t t = (time_t)ts; struct tm v; localtime_r(&t,&v);
  char b[16];
  snprintf(b,sizeof(b),"%02d.%02d.%04d",v.tm_mday,v.tm_mon+1,v.tm_year+1900);
  return String(b);
}

String fmtDateTime(unsigned long ts) {
  if (!ts) return "&mdash;";
  time_t t = (time_t)ts; struct tm v; localtime_r(&t,&v);
  char b[24];
  snprintf(b,sizeof(b),"%02d.%02d.%04d %02d:%02d",v.tm_mday,v.tm_mon+1,v.tm_year+1900,v.tm_hour,v.tm_min);
  return String(b);
}

// books.csv: uid;title;author;year;status;holder;days;issued;due
bool isLate(const String& bl) {
  unsigned long due = strtoul(field(bl,8).c_str(),0,10);
  unsigned long n = nowTs();
  return (field(bl,4) == "borrowed" && due && n && n > due);
}

String stateKey(const String& bl) {
  if (field(bl,4) != "borrowed") return "stock";
  return isLate(bl) ? "late" : "out";
}

String stateTag(const String& bl) {
  String k = stateKey(bl);
  if (k == "stock") return "<span class=\"tag t-ok\">На складе</span>";
  if (k == "late")  return "<span class=\"tag t-late\">Просрочена</span>";
  return "<span class=\"tag t-out\">Выдана</span>";
}

String dash(const String& s) { return s.length() ? s : String("&mdash;"); }

String readerName(const String& uid) {
  String u = findLine(F_USERS, uid);
  return u.length() ? field(u,1) : String("неизвестен");
}

bool isYearStr(const String& s) {
  if (s.length() != 4) return false;
  for (unsigned i = 0; i < 4; i++) if (s[i] < '0' || s[i] > '9') return false;
  int y = s.toInt();
  return (y >= 1400 && y <= 2100);
}

// ==================== МИГРАЦИЯ ====================
void migrateBooks() {
  File f = LittleFS.open(F_BOOKS, "r");
  if (!f) return;
  bool need = false;
  String out = ""; out.reserve(8192);
  while (f.available()) {
    String ln = f.readStringUntil('\n'); ln.trim();
    if (!ln.length()) continue;
    int n = countFields(ln);
    if (n == 9 && (field(ln,3).length() == 0 || isYearStr(field(ln,3)))) {
      out += ln; out += "\n"; continue;
    }
    need = true;
    String nl;
    if (n == 6) {        // uid;title;author;status;holder;days
      nl = field(ln,0)+";"+field(ln,1)+";"+field(ln,2)+";;"
         + field(ln,3)+";"+field(ln,4)+";"+field(ln,5)+";0;0";
    } else if (n == 8) { // uid;title;year;status;holder;days;issued;due
      nl = field(ln,0)+";"+field(ln,1)+";;"+field(ln,2)+";"
         + field(ln,3)+";"+field(ln,4)+";"+field(ln,5)+";"+field(ln,6)+";"+field(ln,7);
    } else if (n == 9) { // uid;title;author;shelf;status;holder;days;issued;due
      nl = field(ln,0)+";"+field(ln,1)+";"+field(ln,2)+";;"
         + field(ln,4)+";"+field(ln,5)+";"+field(ln,6)+";"+field(ln,7)+";"+field(ln,8);
    } else if (n == 18) {
      nl = field(ln,0)+";"+field(ln,1)+";"+field(ln,2)+";"+field(ln,4)+";"
         + field(ln,13)+";"+field(ln,14)+";"+field(ln,15)+";"+field(ln,16)+";"+field(ln,17);
    } else {
      nl = "";
      for (int i = 0; i < 9; i++) { if (i) nl += ";"; nl += field(ln,i); }
    }
    out += nl; out += "\n";
  }
  f.close();
  if (need) {
    f = LittleFS.open(F_BOOKS, "w");
    if (f) { f.print(out); f.close(); }
    Serial.println("books.csv: миграция выполнена");
  }
}

void migrateHistory() {
  File f = LittleFS.open(F_HIST, "r");
  if (!f) return;
  bool need = false;
  String out = ""; out.reserve(8192);
  while (f.available()) {
    String ln = f.readStringUntil('\n'); ln.trim();
    if (!ln.length()) continue;
    int n = countFields(ln);
    if (n < 9) { need = true; for (int i = n; i < 9; i++) ln += ";0"; }
    out += ln; out += "\n";
    int id = field(ln,0).toInt();
    if (id > histCounter) histCounter = id;
  }
  f.close();
  if (need) {
    f = LittleFS.open(F_HIST, "w");
    if (f) { f.print(out); f.close(); }
    Serial.println("history.csv: миграция выполнена");
  }
}

// ==================== RFID ====================
String readTag(MFRC522* r) {
  if (!r->PICC_IsNewCardPresent()) return "";
  if (!r->PICC_ReadCardSerial())   return "";
  String uid = "";
  for (byte i = 0; i < r->uid.size; i++) {
    if (r->uid.uidByte[i] < 0x10) uid += "0";
    uid += String(r->uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  r->PICC_HaltA();
  r->PCD_StopCrypto1();
  return uid;
}

// ==================== SETUP / LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(200);

  if (!LittleFS.begin(true)) { Serial.println("LittleFS: ошибка"); return; }
  migrateBooks();
  migrateHistory();

  SPI.begin();
  pinMode(SS_USER, OUTPUT); digitalWrite(SS_USER, HIGH);
  pinMode(SS_BOOK, OUTPUT); digitalWrite(SS_BOOK, HIGH);
  rfidUser.PCD_Init();
  rfidBook.PCD_Init();
  delay(50);
  Serial.print("RFID A: "); Serial.println(rfidUser.PCD_ReadRegister(MFRC522::VersionReg), HEX);
  Serial.print("RFID B: "); Serial.println(rfidBook.PCD_ReadRegister(MFRC522::VersionReg), HEX);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  configTime(TZ_SEC, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/",         HTTP_GET, pageHome);
  server.on("/register", HTTP_GET, pageRegister);
  server.on("/books",    HTTP_GET, pageBooks);
  server.on("/book",     HTTP_GET, pageBook);
  server.on("/readers",  HTTP_GET, pageReaders);
  server.on("/reader",   HTTP_GET, pageReader);
  server.on("/format",   HTTP_GET, pageFormat);

  server.on("/borrow", HTTP_GET, [](){ server.send_P(200,"text/html",PG_BORROW); });
  server.on("/return", HTTP_GET, [](){ server.send_P(200,"text/html",PG_RETURN); });
  server.on("/style.css", HTTP_GET, [](){ server.send_P(200,"text/css",CSS_FILE); });
  server.on("/app.js",    HTTP_GET, [](){ server.send_P(200,"application/javascript",JS_FILE); });

  server.on("/saveuser", HTTP_POST, saveUser);
  server.on("/savebook", HTTP_POST, saveBook);

  server.on("/api/uid/user", HTTP_GET, [](){ server.send(200,"text/plain",lastUserUID); });
  server.on("/api/uid/book", HTTP_GET, [](){ server.send(200,"text/plain",lastBookUID); });
  server.on("/api/reader", HTTP_GET, apiReader);
  server.on("/api/book",   HTTP_GET, apiBook);
  server.on("/api/borrow", HTTP_GET, apiBorrow);
  server.on("/api/return", HTTP_GET, apiReturn);

  server.onNotFound([](){ server.send(404,"text/plain","404"); });
  server.begin();
  Serial.println("HTTP сервер запущен");
}

void loop() {
  server.handleClient();
  String u = readTag(&rfidUser);
  if (u.length()) { lastUserUID = u; Serial.println("USER: " + u); }
  String b = readTag(&rfidBook);
  if (b.length()) { lastBookUID = b; Serial.println("BOOK: " + b); }
}

// ==================== ОБЗОР ====================
void pageHome() {
  int readers = 0, books = 0, stock = 0, out = 0, late = 0;
  File f = LittleFS.open(F_USERS,"r");
  if (f) { while (f.available()) { String l = f.readStringUntil('\n'); l.trim(); if (l.length()) readers++; } f.close(); }
  f = LittleFS.open(F_BOOKS,"r");
  if (f) {
    while (f.available()) {
      String l = f.readStringUntil('\n'); l.trim();
      if (!l.length()) continue;
      books++;
      if (field(l,4) == "borrowed") { out++; if (isLate(l)) late++; } else stock++;
    }
    f.close();
  }

  beginPage("Обзор","home");
  server.sendContent(R"X(<div class="phead"><div><h2>Обзор</h2><p>Состояние фонда и читателей. Всё обновляется автоматически после каждой операции.</p></div><a class="btn b-red" href="/borrow">Выдать книгу</a></div>)X");

  String s = "<div class=\"grid g5\" style=\"margin-bottom:18px\">";
  s += "<div class=\"stat\"><b>" + String(readers) + "</b><span>Читателей</span></div>";
  s += "<div class=\"stat\"><b>" + String(books)   + "</b><span>Книг в фонде</span></div>";
  s += "<div class=\"stat\"><b>" + String(stock)   + "</b><span>На складе</span></div>";
  s += "<div class=\"stat red\"><b>" + String(out) + "</b><span>Выдано</span></div>";
  s += "<div class=\"stat red\"><b>" + String(late)+ "</b><span>Просрочено</span></div>";
  s += "</div>";
  server.sendContent(s);

  server.sendContent(R"X(<div class="grid g2">
<div class="card"><div class="chd"><h3><span class="bar"></span>Регистрация</h3></div><div class="cbd">
<p class="hint" style="margin:0 0 12px">Новый читатель или новая книга: название, автор, год.</p>
<a class="btn b-ink b-full" href="/register">Открыть регистрацию</a></div></div>
<div class="card"><div class="chd"><h3><span class="bar"></span>Склад и читатели</h3></div><div class="cbd">
<p class="hint" style="margin:0 0 12px">Живой поиск по названию, автору и году, сканирование метки для подсветки.</p>
<div class="acts" style="margin:0"><a class="btn b-out" href="/books">Склад</a><a class="btn b-out" href="/readers">Читатели</a></div></div></div>
</div>)X");

  if (late > 0) {
    server.sendContent(R"X(<div class="card"><div class="chd"><h3><span class="bar"></span>Просроченные книги</h3></div><div class="tbl"><table><thead><tr><th>Книга</th><th>Читатель</th><th>Вернуть до</th><th></th></tr></thead><tbody>)X");
    File fb = LittleFS.open(F_BOOKS,"r");
    if (fb) {
      while (fb.available()) {
        String l = fb.readStringUntil('\n'); l.trim();
        if (!l.length() || !isLate(l)) continue;
        unsigned long due = strtoul(field(l,8).c_str(),0,10);
        unsigned long n = nowTs();
        int days = (int)((n - due) / 86400UL);
        String row = "<tr><td><span class=\"ttl\">" + field(l,1) + "</span><span class=\"sub\">" + dash(field(l,2)) + "</span></td>";
        row += "<td>" + readerName(field(l,5)) + "</td>";
        row += "<td>" + fmtDate(due) + "<span class=\"sub\">просрочка " + String(days) + " дн.</span></td>";
        row += "<td><a class=\"btn b-out b-sm\" href=\"/reader?uid=" + field(l,5) + "\">Читатель</a></td></tr>";
        server.sendContent(row);
      }
      fb.close();
    }
    server.sendContent("</tbody></table></div></div>");
  }

  server.sendContent(R"X(<div class="card"><div class="chd"><h3><span class="bar"></span>Сервис</h3></div><div class="cbd">
<p class="hint" style="margin:0 0 12px">Полное удаление базы читателей, книг и истории операций.</p>
<a class="btn b-out" href="/format" onclick="return confirm('Удалить ВСЕ данные безвозвратно?')">Форматировать память</a></div></div>)X");
  endPage();
}

// ==================== РЕГИСТРАЦИЯ ====================
void sendDatalist(const char* id, int idx) {
  String out = "<datalist id=\""; out += id; out += "\">";
  String seen = "|";
  int n = 0;
  File f = LittleFS.open(F_BOOKS,"r");
  if (f) {
    while (f.available() && n < 30) {
      String l = f.readStringUntil('\n'); l.trim();
      if (!l.length()) continue;
      String v = field(l, idx);
      if (!v.length()) continue;
      String key = "|" + v + "|";
      if (seen.indexOf(key) >= 0) continue;
      seen += v + "|";
      out += "<option value=\"" + v + "\">";
      n++;
    }
    f.close();
  }
  out += "</datalist>";
  server.sendContent(out);
}

void pageRegister() {
  beginPage("Регистрация","reg");
  server.sendContent(R"X(<div class="phead"><div><h2>Регистрация</h2><p>Книга: считать метку, ввести название, автора и год. Автор и год подставляются из ранее введённых.</p></div></div><div id="msgbox"></div>)X");

  if (server.hasArg("ok")) {
    String k = server.arg("ok");
    if (k == "book") server.sendContent("<div class=\"note n-ok\">Книга сохранена. Можно считать следующую метку.</div>");
    if (k == "user") server.sendContent("<div class=\"note n-ok\">Читатель зарегистрирован.</div>");
  }
  if (server.hasArg("err")) {
    server.sendContent("<div class=\"note n-er\">Не заполнено название книги или не считана метка.</div>");
  }

  server.sendContent(R"X(<div class="grid greg">
<div class="card"><div class="chd"><h3><span class="bar"></span>Новый читатель</h3></div><div class="cbd">
<form action="/saveuser" method="POST">
<div class="f"><label>Карта читателя</label><input id="regUser" name="uid" readonly required placeholder="не считана"></div>
<button type="button" class="btn b-scan b-full" onclick="scanTo('/api/uid/user','regUser',this,'uFio')">Считать карту</button>
<div class="sep">Данные</div>
<div class="f"><label>ФИО</label><input id="uFio" name="fio" required placeholder="Иванов Иван Иванович"></div>
<div class="f"><label>Телефон</label><input name="phone" type="tel" required placeholder="+7 900 000-00-00"></div>
<div class="f"><label>Прописка</label><input name="address" required placeholder="город, улица, дом"></div>
<button class="btn b-ink b-full" type="submit">Зарегистрировать</button>
</form></div></div>

<div class="card"><div class="chd"><h3><span class="bar"></span>Новая книга</h3></div><div class="cbd">
<form action="/savebook" method="POST">
<div class="f"><label>Метка книги</label><input id="regBook" name="uid" readonly required placeholder="не считана"></div>
<button type="button" class="btn b-scan b-full" onclick="scanTo('/api/uid/book','regBook',this,'bTitle')">Считать метку</button>
<div class="sep">Описание</div>
<div class="f"><label>Название</label><input id="bTitle" name="title" required placeholder="Название книги"></div>
<div class="f2">
<div class="f"><label>Автор</label><input name="author" list="authors" placeholder="Фамилия И. О."></div>
<div class="f"><label>Год выпуска</label><input name="year" type="number" min="1400" max="2100" list="years" placeholder="2021"></div>
</div>
<button class="btn b-ink b-full" type="submit">Сохранить и добавить следующую</button>
<p class="hint">Метка книги — её уникальный номер. Одинаковых названий может быть сколько угодно, метки разные.</p>
</form></div></div></div>)X");

  sendDatalist("authors", 2);
  sendDatalist("years", 3);
  endPage();
}

void saveUser() {
  String uid = clean(server.arg("uid"), 24);
  String fio = clean(server.arg("fio"), 80);
  if (!uid.length() || !fio.length()) {
    server.sendHeader("Location","/register?err=1"); server.send(303); return;
  }
  if (findLine(F_USERS, uid).length()) {
    server.sendHeader("Location","/reader?uid=" + uid); server.send(303); return;
  }
  File f = LittleFS.open(F_USERS,"a");
  if (f) {
    f.print(uid); f.print(';');
    f.print(fio); f.print(';');
    f.print(clean(server.arg("phone"), 30)); f.print(';');
    f.println(clean(server.arg("address"), 120));
    f.close();
  }
  server.sendHeader("Location","/register?ok=user"); server.send(303);
}

void saveBook() {
  String uid    = clean(server.arg("uid"), 24);
  String title  = clean(server.arg("title"), 100);
  String author = clean(server.arg("author"), 60);
  String year   = clean(server.arg("year"), 4);
  if (!uid.length() || !title.length()) {
    server.sendHeader("Location","/register?err=1"); server.send(303); return;
  }
  if (findLine(F_BOOKS, uid).length()) {
    server.sendHeader("Location","/book?uid=" + uid); server.send(303); return;
  }
  File f = LittleFS.open(F_BOOKS,"a");
  if (f) {
    f.print(uid);    f.print(';');
    f.print(title);  f.print(';');
    f.print(author); f.print(';');
    f.print(year);   f.print(';');
    f.println("stock;;0;0;0");
    f.close();
  }
  server.sendHeader("Location","/register?ok=book"); server.send(303);
}

// ==================== СКЛАД ====================
void pageBooks() {
  beginPage("Склад","books");
  server.sendContent(R"X(<div class="phead"><div><h2>Склад книг</h2><p>Поиск по части названия, автора или года — подсказки появляются сразу при вводе. Стрелки и Enter работают в списке подсказок.</p></div><a class="btn b-out" href="/register">Добавить книгу</a></div>
<div class="panel">
<button class="btn b-scan" onclick="scanFind('/api/uid/book','tblBooks',this)">Сканировать метку</button>
<span>Метка: <span class="res" id="scanRes">&mdash;</span></span>
<div class="search">
<input id="q" autocomplete="off" placeholder="Например: прест, Толстой, 1984" oninput="searchBooks(this.value)" onkeydown="sugKey(event)" onblur="setTimeout(hideSug,150)">
<div class="sug" id="sug"></div>
</div>
<select id="scope" onchange="setScope(this.value)">
<option value="all">Везде</option>
<option value="title">Название</option>
<option value="author">Автор</option>
<option value="year">Год</option>
</select>
<div class="chips">
<button class="chip on" onclick="setSt('all',this)">Все</button>
<button class="chip" onclick="setSt('stock',this)">На складе</button>
<button class="chip" onclick="setSt('out',this)">Выданы</button>
<button class="chip" onclick="setSt('late',this)">Просрочены</button>
</div>
<span class="found">Найдено: <span id="found">0</span></span>
</div>
<div class="card"><div class="tbl"><table id="tblBooks"><thead><tr><th>Метка</th><th>Книга</th><th>Год</th><th>Статус</th><th></th></tr></thead><tbody>)X");

  File f = LittleFS.open(F_BOOKS,"r");
  bool any = false;
  if (f) {
    while (f.available()) {
      String l = f.readStringUntil('\n'); l.trim();
      if (!l.length()) continue;
      any = true;
      String uid = field(l,0);
      String row = "<tr id=\"row_" + uid + "\" data-uid=\"" + uid + "\" data-st=\"" + stateKey(l)
                 + "\" data-t=\"" + field(l,1) + "\" data-a=\"" + field(l,2) + "\" data-y=\"" + field(l,3) + "\">";
      row += "<td class=\"uid\">" + uid + "</td>";
      row += "<td><span class=\"ttl\">" + field(l,1) + "</span><span class=\"sub\">" + dash(field(l,2)) + "</span></td>";
      row += "<td>" + dash(field(l,3)) + "</td>";
      row += "<td>" + stateTag(l);
      if (field(l,4) == "borrowed") {
        row += "<span class=\"sub\">" + readerName(field(l,5)) + " &middot; до " + fmtDate(strtoul(field(l,8).c_str(),0,10)) + "</span>";
      }
      row += "</td><td><a class=\"btn b-out b-sm\" href=\"/book?uid=" + uid + "\">Подробнее</a></td></tr>";
      server.sendContent(row);
    }
    f.close();
  }
  if (!any) server.sendContent("<tr><td colspan=\"5\" class=\"none\">Фонд пуст. Добавьте первую книгу.</td></tr>");
  server.sendContent("<tr id=\"noMatch\" style=\"display:none\"><td colspan=\"5\" class=\"none\">Ничего не найдено</td></tr>");
  server.sendContent("</tbody></table></div></div>");
  endPage();
}

// ==================== КНИГА ====================
void pageBook() {
  String uid = server.arg("uid");
  String b = findLine(F_BOOKS, uid);
  beginPage("Книга","books");

  if (!b.length()) {
    server.sendContent("<a class=\"back\" href=\"/books\">&larr; Назад к складу</a><div class=\"note n-er\">Книга не найдена.</div>");
    endPage(); return;
  }

  server.sendContent("<a class=\"back\" href=\"/books\">&larr; Назад к складу</a>");
  String h = "<div class=\"card\"><div class=\"chd\"><h3><span class=\"bar\"></span>" + field(b,1) + "</h3>" + stateTag(b) + "</div><div class=\"cbd\">";
  h += "<div class=\"dl\">";
  h += "<div><dt>Метка</dt><dd class=\"uid\">" + uid + "</dd></div>";
  h += "<div><dt>Автор</dt><dd>" + dash(field(b,2)) + "</dd></div>";
  h += "<div><dt>Год выпуска</dt><dd>" + dash(field(b,3)) + "</dd></div>";
  h += "</div>";
  server.sendContent(h);

  if (field(b,4) == "borrowed") {
    unsigned long due = strtoul(field(b,8).c_str(),0,10);
    unsigned long iss = strtoul(field(b,7).c_str(),0,10);
    String box = "<div class=\"box\"><b>На руках.</b> Читатель: " + readerName(field(b,5)) + "<br>";
    box += "Выдана: " + fmtDate(iss) + " &middot; срок " + field(b,6) + " дн. &middot; вернуть до <b>" + fmtDate(due) + "</b>";
    if (isLate(b)) box += " &middot; <b>просрочена</b>";
    box += "<div class=\"acts\"><a class=\"btn b-red b-sm\" href=\"/reader?uid=" + field(b,5) + "\">Карточка читателя</a><a class=\"btn b-out b-sm\" href=\"/return\">Оформить возврат</a></div></div>";
    server.sendContent(box);
  } else {
    server.sendContent("<div class=\"acts\"><a class=\"btn b-red\" href=\"/borrow\">Выдать книгу</a></div>");
  }
  server.sendContent("</div></div>");

  server.sendContent(R"X(<div class="card"><div class="chd"><h3><span class="bar"></span>История книги</h3></div><div class="tbl"><table><thead><tr><th>N</th><th>Дата</th><th>Операция</th><th>Читатель</th><th>Срок</th></tr></thead><tbody>)X");
  File f = LittleFS.open(F_HIST,"r");
  bool any = false;
  if (f) {
    while (f.available()) {
      String l = f.readStringUntil('\n'); l.trim();
      if (!l.length() || field(l,3) != uid) continue;
      any = true;
      bool bor = (field(l,5) == "borrow");
      String tag = bor ? "<span class=\"tag t-out\">Выдача</span>" : "<span class=\"tag t-ok\">Возврат</span>";
      String dur = bor ? field(l,6) + " дн." : String("&mdash;");
      String row = "<tr><td>" + field(l,0) + "</td><td>" + fmtDateTime(strtoul(field(l,7).c_str(),0,10)) + "</td>";
      row += "<td>" + tag + "</td><td>" + field(l,2) + "</td><td>" + dur + "</td></tr>";
      server.sendContent(row);
    }
    f.close();
  }
  if (!any) server.sendContent("<tr><td colspan=\"5\" class=\"none\">Операций пока не было</td></tr>");
  server.sendContent("</tbody></table></div></div>");
  endPage();
}

// ==================== ЧИТАТЕЛИ ====================
void pageReaders() {
  beginPage("Читатели","readers");
  server.sendContent(R"X(<div class="phead"><div><h2>Читатели</h2><p>Сканирование карты подсветит нужную строку в списке.</p></div><a class="btn b-out" href="/register">Добавить читателя</a></div>
<div class="panel">
<button class="btn b-scan" onclick="scanFind('/api/uid/user','tblReaders',this)">Сканировать карту</button>
<span>Карта: <span class="res" id="scanRes">&mdash;</span></span>
<input style="flex:1;min-width:200px" placeholder="Поиск по ФИО или телефону" oninput="filterRows('tblReaders',this.value)">
</div>
<div class="card"><div class="tbl"><table id="tblReaders"><thead><tr><th>Карта</th><th>ФИО</th><th>Телефон</th><th>На руках</th><th></th></tr></thead><tbody>)X");

  File f = LittleFS.open(F_USERS,"r");
  bool any = false;
  if (f) {
    while (f.available()) {
      String l = f.readStringUntil('\n'); l.trim();
      if (!l.length()) continue;
      any = true;
      String uid = field(l,0);
      int cnt = 0, lt = 0;
      File fb = LittleFS.open(F_BOOKS,"r");
      if (fb) {
        while (fb.available()) {
          String bl = fb.readStringUntil('\n'); bl.trim();
          if (!bl.length()) continue;
          if (field(bl,4) == "borrowed" && field(bl,5) == uid) { cnt++; if (isLate(bl)) lt++; }
        }
        fb.close();
      }
      String hand = String(cnt);
      if (lt) hand += " <span class=\"tag t-late\">просрочка " + String(lt) + "</span>";
      String row = "<tr id=\"row_" + uid + "\" data-st=\"r\"><td class=\"uid\">" + uid + "</td>";
      row += "<td class=\"ttl\">" + field(l,1) + "</td><td>" + field(l,2) + "</td><td>" + hand + "</td>";
      row += "<td><a class=\"btn b-out b-sm\" href=\"/reader?uid=" + uid + "\">Подробнее</a></td></tr>";
      server.sendContent(row);
    }
    f.close();
  }
  if (!any) server.sendContent("<tr><td colspan=\"5\" class=\"none\">Читателей пока нет</td></tr>");
  server.sendContent("</tbody></table></div></div>");
  endPage();
}

// ==================== ЧИТАТЕЛЬ ====================
void pageReader() {
  String uid = server.arg("uid");
  String u = findLine(F_USERS, uid);
  beginPage("Читатель","readers");

  if (!u.length()) {
    server.sendContent("<a class=\"back\" href=\"/readers\">&larr; Назад</a><div class=\"note n-er\">Читатель не найден.</div>");
    endPage(); return;
  }

  server.sendContent("<a class=\"back\" href=\"/readers\">&larr; Назад к читателям</a>");
  String h = "<div class=\"card\"><div class=\"chd\"><h3><span class=\"bar\"></span>" + field(u,1) + "</h3></div><div class=\"cbd\"><div class=\"dl\">";  h += "<div><dt>Карта</dt><dd class=\"uid\">" + uid + "</dd></div>";
  h += "<div><dt>Телефон</dt><dd>" + dash(field(u,2)) + "</dd></div>";
  h += "<div><dt>Прописка</dt><dd>" + dash(field(u,3)) + "</dd></div></div>";
  server.sendContent(h);
  server.sendContent(R"X(<div class="sep">Книги на руках</div><div class="tbl"><table><thead><tr><th>Метка</th><th>Книга</th><th>Выдана</th><th>Вернуть до</th><th></th></tr></thead><tbody>)X");

  File fb = LittleFS.open(F_BOOKS,"r");
  bool has = false;
  if (fb) {
    while (fb.available()) {
      String l = fb.readStringUntil('\n'); l.trim();
      if (!l.length()) continue;
      if (field(l,4) != "borrowed" || field(l,5) != uid) continue;
      has = true;
      String row = "<tr><td class=\"uid\">" + field(l,0) + "</td>";
      row += "<td><span class=\"ttl\">" + field(l,1) + "</span><span class=\"sub\">" + dash(field(l,2));
      if (field(l,3).length()) row += " &middot; " + field(l,3);
      row += "</span></td>";
      row += "<td>" + fmtDate(strtoul(field(l,7).c_str(),0,10)) + "</td>";
      row += "<td>" + fmtDate(strtoul(field(l,8).c_str(),0,10)) + " " + stateTag(l) + "</td>";
      row += "<td><a class=\"btn b-out b-sm\" href=\"/book?uid=" + field(l,0) + "\">Книга</a></td></tr>";
      server.sendContent(row);
    }
    fb.close();
  }
  if (!has) server.sendContent("<tr><td colspan=\"5\" class=\"none\">Нет книг на руках</td></tr>");
  server.sendContent("</tbody></table></div></div></div>");

  server.sendContent(R"X(<div class="card"><div class="chd"><h3><span class="bar"></span>История операций</h3></div><div class="tbl"><table><thead><tr><th>N</th><th>Дата</th><th>Книга</th><th>Операция</th><th>Срок</th></tr></thead><tbody>)X");
  File fh = LittleFS.open(F_HIST,"r");
  bool any = false;
  if (fh) {
    while (fh.available()) {
      String l = fh.readStringUntil('\n'); l.trim();
      if (!l.length() || field(l,1) != uid) continue;
      any = true;
      bool bor = (field(l,5) == "borrow");
      String tag = bor ? "<span class=\"tag t-out\">Выдача</span>" : "<span class=\"tag t-ok\">Возврат</span>";
      String dur = bor ? field(l,6) + " дн." : String("&mdash;");
      String row = "<tr><td>" + field(l,0) + "</td><td>" + fmtDateTime(strtoul(field(l,7).c_str(),0,10)) + "</td>";
      row += "<td>" + field(l,4) + "</td><td>" + tag + "</td><td>" + dur + "</td></tr>";
      server.sendContent(row);
    }
    fh.close();
  }
  if (!any) server.sendContent("<tr><td colspan=\"5\" class=\"none\">История пуста</td></tr>");
  server.sendContent("</tbody></table></div></div>");
  endPage();
}

// ==================== API ====================
void apiReader() {
  String u = findLine(F_USERS, server.arg("uid"));
  if (!u.length()) { server.send(200,"text/html","<b style=\"color:#b2152a\">Читатель не зарегистрирован</b>"); return; }
  server.send(200,"text/html","<b>" + field(u,1) + "</b><br>" + field(u,2));
}

void apiBook() {
  String b = findLine(F_BOOKS, server.arg("uid"));
  if (!b.length()) { server.send(200,"text/html","<b style=\"color:#b2152a\">Книга не зарегистрирована</b>"); return; }
  String s = "<b>" + field(b,1) + "</b>";
  if (field(b,2).length()) s += "<br>" + field(b,2);
  if (field(b,3).length()) s += "<span class=\"sub\">Год: " + field(b,3) + "</span>";
  s += "<br>" + stateTag(b);
  if (field(b,4) == "borrowed") {
    s += " " + readerName(field(b,5)) + "<br><span class=\"sub\">до " + fmtDate(strtoul(field(b,8).c_str(),0,10)) + "</span>";
  }
  server.send(200,"text/html",s);
}

void addHistory(const String& rUID, const String& rName, const String& bUID,
                const String& bTitle, const char* act, const String& days,
                unsigned long ts, unsigned long due) {
  histCounter++;
  File f = LittleFS.open(F_HIST,"a");
  if (!f) return;
  f.print(histCounter); f.print(';');
  f.print(rUID);   f.print(';');
  f.print(rName);  f.print(';');
  f.print(bUID);   f.print(';');
  f.print(bTitle); f.print(';');
  f.print(act);    f.print(';');
  f.print(days);   f.print(';');
  f.print(ts);     f.print(';');
  f.println(due);
  f.close();
}

void apiBorrow() {
  String rUID = server.arg("r"), bUID = server.arg("b"), days = server.arg("d");
  String u = findLine(F_USERS, rUID);
  if (!u.length()) { server.send(200,"text/html","<div class=\"note n-er\">Читатель не зарегистрирован</div>"); return; }
  String b = findLine(F_BOOKS, bUID);
  if (!b.length()) { server.send(200,"text/html","<div class=\"note n-er\">Книга не зарегистрирована</div>"); return; }
  if (field(b,4) == "borrowed") {
    server.send(200,"text/html","<div class=\"note n-er\">Книга уже на руках у: " + readerName(field(b,5)) + "</div>"); return;
  }
  int d = days.toInt();
  if (d <= 0 || d > 365) d = 14;

  unsigned long ts = nowTs();
  unsigned long due = ts ? ts + (unsigned long)d * 86400UL : 0;

  String nl = field(b,0)+";"+field(b,1)+";"+field(b,2)+";"+field(b,3)
            + ";borrowed;" + rUID + ";" + String(d) + ";" + String(ts) + ";" + String(due);
  replaceLine(F_BOOKS, bUID, nl);
  addHistory(rUID, field(u,1), bUID, field(b,1), "borrow", String(d), ts, due);
  lastUserUID = ""; lastBookUID = "";

  String msg = "<div class=\"note n-ok\">Книга &laquo;" + field(b,1) + "&raquo; выдана: " + field(u,1)
             + ". Срок " + String(d) + " дн.";
  if (due) msg += ", вернуть до " + fmtDate(due);
  msg += ".</div>";
  server.send(200,"text/html",msg);
}

void apiReturn() {
  String rUID = server.arg("r"), bUID = server.arg("b");
  String u = findLine(F_USERS, rUID);
  if (!u.length()) { server.send(200,"text/html","<div class=\"note n-er\">Читатель не зарегистрирован</div>"); return; }
  String b = findLine(F_BOOKS, bUID);
  if (!b.length()) { server.send(200,"text/html","<div class=\"note n-er\">Книга не зарегистрирована</div>"); return; }
  if (field(b,4) != "borrowed") {
    server.send(200,"text/html","<div class=\"note n-er\">Книга уже на складе</div>"); return;
  }
  if (field(b,5) != rUID) {
    server.send(200,"text/html","<div class=\"note n-er\">Книга числится за: " + readerName(field(b,5)) + "</div>"); return;
  }

  bool wasLate = isLate(b);
  unsigned long ts = nowTs();
  String nl = field(b,0)+";"+field(b,1)+";"+field(b,2)+";"+field(b,3)+";stock;;0;0;0";
  replaceLine(F_BOOKS, bUID, nl);
  addHistory(rUID, field(u,1), bUID, field(b,1), "return", "0", ts, 0);
  lastUserUID = ""; lastBookUID = "";

  String msg = "<div class=\"note n-ok\">Книга &laquo;" + field(b,1) + "&raquo; принята на склад от: " + field(u,1) + ".";
  if (wasLate) msg += " Возврат с просрочкой (отмечено в истории).";
  msg += "</div>";
  server.send(200,"text/html",msg);
}

// ==================== СЕРВИС ====================
void pageFormat() {
  LittleFS.format();
  histCounter = 0;
  lastUserUID = ""; lastBookUID = "";
  beginPage("Сервис","home");
  server.sendContent(R"X(<div class="card"><div class="chd"><h3><span class="bar"></span>Память очищена</h3></div><div class="cbd">
<p class="hint" style="margin:0 0 12px">База читателей, книг и история удалены.</p>
<a class="btn b-ink" href="/">На главную</a></div></div>)X");
  endPage();
}