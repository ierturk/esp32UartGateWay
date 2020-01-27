(function(e){function t(t){for(var r,c,s=t[0],i=t[1],l=t[2],f=0,p=[];f<s.length;f++)c=s[f],Object.prototype.hasOwnProperty.call(o,c)&&o[c]&&p.push(o[c][0]),o[c]=0;for(r in i)Object.prototype.hasOwnProperty.call(i,r)&&(e[r]=i[r]);u&&u(t);while(p.length)p.shift()();return a.push.apply(a,l||[]),n()}function n(){for(var e,t=0;t<a.length;t++){for(var n=a[t],r=!0,s=1;s<n.length;s++){var i=n[s];0!==o[i]&&(r=!1)}r&&(a.splice(t--,1),e=c(c.s=n[0]))}return e}var r={},o={app:0},a=[];function c(t){if(r[t])return r[t].exports;var n=r[t]={i:t,l:!1,exports:{}};return e[t].call(n.exports,n,n.exports,c),n.l=!0,n.exports}c.m=e,c.c=r,c.d=function(e,t,n){c.o(e,t)||Object.defineProperty(e,t,{enumerable:!0,get:n})},c.r=function(e){"undefined"!==typeof Symbol&&Symbol.toStringTag&&Object.defineProperty(e,Symbol.toStringTag,{value:"Module"}),Object.defineProperty(e,"__esModule",{value:!0})},c.t=function(e,t){if(1&t&&(e=c(e)),8&t)return e;if(4&t&&"object"===typeof e&&e&&e.__esModule)return e;var n=Object.create(null);if(c.r(n),Object.defineProperty(n,"default",{enumerable:!0,value:e}),2&t&&"string"!=typeof e)for(var r in e)c.d(n,r,function(t){return e[t]}.bind(null,r));return n},c.n=function(e){var t=e&&e.__esModule?function(){return e["default"]}:function(){return e};return c.d(t,"a",t),t},c.o=function(e,t){return Object.prototype.hasOwnProperty.call(e,t)},c.p="/";var s=window["webpackJsonp"]=window["webpackJsonp"]||[],i=s.push.bind(s);s.push=t,s=s.slice();for(var l=0;l<s.length;l++)t(s[l]);var u=i;a.push([0,"chunk-vendors"]),n()})({0:function(e,t,n){e.exports=n("56d7")},"034f":function(e,t,n){"use strict";var r=n("85ec"),o=n.n(r);o.a},"24df":function(e,t,n){},"56d7":function(e,t,n){"use strict";n.r(t);var r=n("2b0e"),o=function(){var e=this,t=e.$createElement,n=e._self._c||t;return n("div",{attrs:{id:"app"}},[n("welcome")],1)},a=[],c=function(){var e=this,t=e.$createElement,n=e._self._c||t;return n("b-container",[n("b-row",{staticClass:"justify-content-md-center"},[n("b-col",{attrs:{col:"",lg:"2"}}),n("b-col",{attrs:{cols:"6"}},[n("div",[n("h1",{staticClass:"display-2"},[e._v(" marmatek ")]),n("b-card",{attrs:{title:"ESP32 Connection Test","bg-variant":"light"}},[n("div",{staticClass:"text-left"},[n("h5",[e._v("Board Temperature: "+e._s(e.msg)+" ")]),n("div",{staticClass:"spacer10"}),n("b-form-textarea",{attrs:{id:"textarea-large",size:"lg",placeholder:"Serial Data",readonly:"",rows:6,"max-rows":6},model:{value:e.serial,callback:function(t){e.serial=t},expression:"serial"}}),n("div",{staticClass:"spacer10"}),n("b-button",{attrs:{href:"#",variant:"primary"}},[e._v("Go somewhere")])],1)])],1)]),n("b-col",{attrs:{col:"",lg:"2"}})],1)],1)},s=[],i=n("2f62"),l={name:"Welcome",data:function(){return{msg:"",serial:""}},computed:Object(i["b"])(["msgval","sdata"]),watch:{msgval:function(e,t){this.msg=e.temp,null!=e.msg&&(this.serial+=e.msg)}},created:function(){}},u=l,f=(n("da20"),n("2877")),p=Object(f["a"])(u,c,s,!1,null,"0fec7396",null),d=p.exports,g={name:"app",components:{Welcome:d}},m=g,v=(n("034f"),Object(f["a"])(m,o,a,!1,null,null,null)),b=v.exports,y=n("5f5b"),O=(n("f9e3"),n("2dd8"),n("96cf"),n("1da1"));r["default"].use(i["a"]);var h=new i["a"].Store({state:{authenticated:!1,socket:{isConnected:!1,message:"",reconnectError:!1},msgval:null,count:0},getters:{isConnected:function(e){return e.socket.isConnected}},mutations:{UPDATE_MSGVAL:function(e,t){e.msgval=t},SOCKET_ONOPEN:function(e,t){r["default"].prototype.$socket=t.currentTarget,e.socket.isConnected=!0,r["default"].prototype.$socket.binaryType="arraybuffer",console.log("Socket opened",r["default"].prototype.$socket)},SOCKET_ONCLOSE:function(e,t){e.socket.isConnected=!1,console.log("Socket closed")},SOCKET_ONERROR:function(e,t){console.log("Socket error"),console.error(e,t)},SOCKET_ONMESSAGE:function(e,t){if("string"===typeof t.data||t.data instanceof String){e.socket.message=t.data;var n=JSON.parse(t.data);e.msgval=n,e.count=e.count+1}else console.log("binary message received",t)},SOCKET_RECONNECT:function(e,t){console.info(e,t)},SOCKET_RECONNECT_ERROR:function(e){e.socket.reconnectError=!0}},actions:{sendMessage:function(e,t){r["default"].prototype.$socket.send(JSON.stringify(t)),console.log("sendMessage",JSON.stringify(t))},updateZvals:function(e,t){var n=e.commit;n("UPDATE_MSGVAL",t)},connectWebsocket:function(){var e=Object(O["a"])(regeneratorRuntime.mark((function e(t){return regeneratorRuntime.wrap((function(e){while(1)switch(e.prev=e.next){case 0:return t.commit,console.log("trying connect websocket"),e.prev=2,e.next=5,r["default"].prototype.$connect();case 5:e.next=10;break;case 7:e.prev=7,e.t0=e["catch"](2),console.error(e.t0);case 10:case"end":return e.stop()}}),e,null,[[2,7]])})));function t(t){return e.apply(this,arguments)}return t}()}}),S=n("b408"),w=n.n(S);r["default"].use(y["a"]),r["default"].config.productionTip=!1,r["default"].use(w.a,"ws://192.168.4.1:8081",{store:h}),new r["default"]({el:"#app",store:h,render:function(e){return e(b)}})},"85ec":function(e,t,n){},da20:function(e,t,n){"use strict";var r=n("24df"),o=n.n(r);o.a}});