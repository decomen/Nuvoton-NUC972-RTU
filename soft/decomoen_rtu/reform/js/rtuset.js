

var CONFIG_PATH			= "media/nand/cfg";
var UPDATE_PATH			= "media/update";

var ENET_TCPIP_NUM		= 8;
var GPRS_TCPIP_NUM		= 4;
var EXT_VAR_LIMIT		= 256;
var BOARD_ZIGBEE_UART	= 3;

// ---------常量表------------
var PROTO_DEV_RS1		= 0;
var PROTO_DEV_RS2		= 1;
var PROTO_DEV_RS3		= 2;
var PROTO_DEV_RS4		= 3;
var PROTO_DEV_NET		= 4;
var PROTO_DEV_ZIGBEE	= 5;
var PROTO_DEV_GPRS		= 6;
var PROTO_DEV_LORA		= 7;
var PROTO_DEV_RTU_SELF	= 100; 
var PROTO_DEV_RTU_SELF_MID	= 200; 

var PROTO_DEV_RS_MAX	= 3;

// proto_uart_type_e
var PROTO_MODBUS_RTU	= 0;	//modbus RTU
var PROTO_MODBUS_ASCII	= 1;	//modbus ASCII
var PROTO_DLT645		= 2;	// dlt645 2007
var PROTO_DLT645_1997	= 3;	// dlt645 1997
var PROTO_DUST			= 4;	// 粉尘协议
var PROTO_LUA			= 20;   // lua

// proto_tcpip_type_e
var PROTO_MODBUS_TCP	= 0;	// modbus TCP
var PROTO_CC_BJDC		= 1;	// 北京数据采集协议
var PROTO_MODBUS_RTU_OVER_TCP	= 2;	// modbus rtu over TCP
var PROTO_HJT212		= 3;	// hjt212
var PROTO_DM101			= 4;	// dm101
// ---------------------------

var TCP_IP_M_NORMAL		= 0x00; // 正常通信模式(网关)
var TCP_IP_M_XFER		= 0x01; // 转发模式 

var XFER_M_GW			= 0; // 指定协议网关模式 (指定端口:支持zigbee)
var XFER_M_TRT			= 1; // 无协议透明传输 (指定端口)

var ZGB_TM_GW			= 0; // 网关模式 (采集变量采集)
var ZGB_TM_TRT			= 1; // 无协议透明传输 (指定端口)
var ZGB_TM_DTU			= 2; // 组网转发模式 (转发到串口)

var NET_ADAPTER_WIRED 		= 0;   	//本地网络
var NET_ADAPTER_WIRELESS 	= 1;  	//GPRS/LTE
var GPRS_OR_NBIOT	= "GPRS/LTE";

var xTcpipCfgList = new Array();
var xUartCfgList = new Array();
var xVarManageExtDataBase = new Array();
var xProtoDevList = new Array();
var xUpProtoDevList = new Array();
//脚本执行器
var xLuaList = new Array();

function __is_gprs()
{
	return (GPRS_OR_NBIOT == "GPRS/LTE");
}

function __proto_is_master_fixed(proto_type)
{
	return (proto_type == PROTO_DLT645 || proto_type == PROTO_DLT645_1997 || proto_type == PROTO_DUST);
}

function __proto_is_modbus(dev_type, proto_type)
{
	if (dev_type <= PROTO_DEV_RS_MAX) {
		return (proto_type <= PROTO_MODBUS_ASCII);
	} else if (dev_type <= PROTO_DEV_NET) {
		return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
	} else if (dev_type <= PROTO_DEV_ZIGBEE) {
		return (proto_type <= PROTO_MODBUS_ASCII);
	} else if (dev_type <= PROTO_DEV_GPRS) {
		return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
	}

	return false;
}

function __proto_is_dlt645_2007(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645);
}

function __proto_is_dlt645_1997(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645_1997);
}

function __proto_is_dlt645(dev_type, proto_type)
{
	return (__proto_is_dlt645_2007(dev_type, proto_type) || __proto_is_dlt645_1997(dev_type, proto_type));
}

function __proto_is_devself(dev_type)
{
	return (dev_type == PROTO_DEV_RTU_SELF);
}

function __proto_is_devself_mid(dev_type)
{
	return (dev_type == PROTO_DEV_RTU_SELF_MID);
}

function __proto_is_dust(dev_type, proto_type)
{
	return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DUST);
}

function createXMLHttpRequest() {
	var request = false;
	if(window.ActiveXObject) {
		var versions = ['Microsoft.XMLHTTP', 'MSXML.XMLHTTP', 'Microsoft.XMLHTTP', 'Msxml2.XMLHTTP.7.0', 'Msxml2.XMLHTTP.6.0', 'Msxml2.XMLHTTP.5.0', 'Msxml2.XMLHTTP.4.0', 'MSXML2.XMLHTTP.3.0', 'MSXML2.XMLHTTP'];
		for(var i=0; i<versions.length; i++) {
			try {
				request = new ActiveXObject(versions[i]);
				if(request) {
					return request;
				}
			} catch(e) {}
		}
	} else if(window.XMLHttpRequest) {
		request = new XMLHttpRequest();
	}
	return request;
}

function _tostr_(_str) { return ''+(_str!=null?_str:''); }
function _tonum_(_num) { return _num!=null?Number(_num):0; }


function getNumber( id )
{
	return Number(window.document.getElementById(id).value);
}

function getChecked( id )
{
	return window.document.getElementById(id).checked;
}

function getValue( id )
{
	return window.document.getElementById(id).value;
}

function setCheckBoxEnable(id, val){
	if(window.document.getElementById(id)){
		window.document.getElementById(id).checked = (Number(val)!=0);
	}
}

function setSelectValueByClass(_class, val){
	 $("."+_class).find("option").removeAttr("selected");
	 $("."+_class).find("option[value="+val+"]").attr("selected",true)
}
function setValueByclass(_class, val){
	$("#"+_class).val(val);
}

function setValue( id, val )
{
	if(window.document.getElementById(id)){
		window.document.getElementById(id).value = val;
	}
}

function setEnable( id, enable )
{
	if(window.document.getElementById(id)){
		window.document.getElementById(id).disabled=!enable;
	}
}

function setDisplay( id, display )
{
	window.document.getElementById(id).style.display=(display?"":"none");
}

function setVisible( id, visible )
{
	window.document.getElementById(id).style.visibility=visible?"visible":"hidden";
}

function MyGetJSON(msg,url,node,data,callback)
{
	var flag = (msg != "" && msg != null);
	if(flag) Show(msg);
	if(data!=null&&data.length>0) {
		data = data.replace(/\%/g, '%25');
		data = data.replace(/\#/g, '%23');
		data = data.replace(/\&/g, '%26');
		data = data.replace(/\+/g, '%2B');
		//data = data.replace(/\\/g, '%2F');
		data = data.replace(/\=/g, '%3D');
		data = data.replace(/\?/g, '%3F');
		data = node+'='+data;
	} else {
		data = '';
	}
	$.ajax({ 
	url: url + data + "&__time="+Date.parse( new Date()),  
	async: true,
	cache: false,
	dataType: 'json', 
	//data: data, 
	success: function(data){
		if(flag) Close();
		if(data.ret != null ) {
			if( data.ret == 401 ) {
				window.location.href="http://"+window.location.host+"/login.html";
			} else if( data.ret == 402 ) {
				window.location.href="http://"+window.location.host+"/reg.html";
			}
		}
		if(callback!=null) callback(data);
	},
	timeout: 5000,
	error: function( ){
		if(flag) Close();
	}
	});
}

function MyGetJSONWithArg(msg,url,data,callback)
{
	MyGetJSON(msg,url,'arg',data,callback)
}

function initSetting()
{
	getDevInfo();
	for(var i = 0; i <= PROTO_DEV_RS_MAX; i++) {
		onUartCfgMsChange(i);
	}
	for(var i = 0; i < 8; i++) {
		onAiTypeChange(i);
	}
}

function onUartCfgMsChange(n)
{
	var po = getNumber('uart' + n + '_cfg_proto');
	var ms = getNumber('uart' + n + '_cfg_ms');
	
	if( ms != null && po != null && po == 0 && ms == 0 ) {
		setVisible( 'uart'+ n + '_cfg_addr_lb', true );
		setVisible( 'uart'+ n + '_cfg_addr', true );
	} else {
		setVisible( 'uart'+ n + '_cfg_addr_lb', false );
		setVisible( 'uart'+ n + '_cfg_addr', false );
	}
}

function setUartHtml(n, bd, ut, po, py, ms, ad)
{
	if( bd != null ) setValue('uart' + n + '_cfg_baud', bd); 
	if( ut != null ) setValue('uart' + n + '_cfg_mode', ut); 
	if( po != null ) setValue('uart' + n + '_cfg_proto', po); 
	if( py != null ) setValue('uart' + n + '_cfg_parity', py); 
	if( ms != null ) setValue('uart' + n + '_cfg_ms', ms); 
	if( ad != null ) setValue('uart' + n + '_cfg_addr', ad);
	
	onUartCfgMsChange(n);
}

function setUartCfg(i)
{
	var setval = {
		n:Number(i), 
		bd:getNumber('uart' + i + '_cfg_baud'), 
		ut:getNumber('uart' + i + '_cfg_mode'), 
		po:getNumber('uart' + i + '_cfg_proto'), 
		py:getNumber('uart' + i + '_cfg_parity'), 
		ms:getNumber('uart' + i + '_cfg_ms'), 
		ad:getNumber('uart' + i + '_cfg_addr')
	};
	MyGetJSONWithArg("正在设置串口参数,请稍后...","/cgi-bin/setUartCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			alert( "设置成功" );
		} else {
			alert("设置失败,请重试");
		}
	});
}

function Show(message){
	var shield = document.createElement("DIV");//产生一个背景遮罩层
	shield.id = "shield";
	shield.style.position = "absolute";
	shield.style.left = "0px";
	shield.style.top = "0px";
	shield.style.width = "100%";
	shield.style.height = ((document.documentElement.clientHeight>document.documentElement.scrollHeight)?document.documentElement.clientHeight:document.documentElement.scrollHeight)+"px";
	shield.style.background = "#333";
	shield.style.textAlign = "center";
	shield.style.zIndex = "10000";
	shield.style.filter = "alpha(opacity=0)";
	shield.style.opacity = 0;

	var alertFram = document.createElement("DIV");//产生一个提示框
	var height="40px";
	alertFram.id="alertFram";
	alertFram.style.position = "absolute";
	alertFram.style.width = "200px";
	alertFram.style.height = height;
	alertFram.style.left = "45%";
	alertFram.style.top = "50%";
	alertFram.style.background = "#fff";
	alertFram.style.textAlign = "center";
	alertFram.style.lineHeight = height;
	alertFram.style.zIndex = "100001";

   strHtml =" <div style=\"width:100%; border:#58a3cb solid 1px; text-align:center;\">";
	if (typeof(message)=="undefined"){
		strHtml+=" 正在操作, 请稍候...";
	} 
	else{
		strHtml+=message;
	}
	strHtml+=" </div>";

	alertFram.innerHTML=strHtml;
	document.body.appendChild(alertFram);
	document.body.appendChild(shield);


	var c = 0;
	
	var ad = setInterval(function(){
		doAlpha(c,shield)
	},1);//渐变效果
	document.body.onselectstart = function(){return false;}
	document.body.oncontextmenu = function(){return false;}
}
function doAlpha(c,shield){
	if (++c > 20){clearInterval(ad);return 0;}
	setOpacity(shield,c);
	}

  function setOpacity(obj,opacity){
		if(opacity>=1)opacity=opacity/100;
		try{ obj.style.opacity=opacity; }catch(e){}
		try{ 
			if(obj.filters.length>0&&obj.filters("alpha")){
			obj.filters("alpha").opacity=opacity*100;
			}else{
			obj.style.filter="alpha(opacity=\""+(opacity*100)+"\")";
			}
		}
		catch(e){}
	}

function Close(){
	var shield= window.document.getElementById("shield");
	var alertFram= window.document.getElementById("alertFram");
	if(shield!=null) {
		document.body.removeChild(shield);
	}
	if(alertFram!=null) {
		document.body.removeChild(alertFram);
	} 
	document.body.onselectstart = function(){return true};
	document.body.oncontextmenu = function(){return true};
}

function getAllUartCfg()
{
	MyGetJSONWithArg("正在获取串口配置,请稍后...","/cgi-bin/getUartCfg?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			xUartCfgList = res.list.concat();
			for( var n = 0; n < res.list.length; n++ ) {
				setUartHtml( 
					n, 
					res.list[n].bd, 
					res.list[n].ut, 
					res.list[n].po, 
					res.list[n].py, 
					res.list[n].ms, 
					res.list[n].ad
				);
			}
		} else {
			alert("获取失败,请重试");
		}
	});
}

function getUartCfg(i)
{
	var setval = { n:Number(i) };
	MyGetJSONWithArg("正在获取串口配置,请稍后...","/cgi-bin/getUartCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			setUartHtml( i, res.bd, res.ut, res.po, res.py, res.ms, res.ad );
		} else {
			alert("获取失败,请重试");
		}
	});
}

function refreshAllVarManageExtDataBase(index)
{
	refreshProtoDevList( xProtoDevList, "var_ext_devtype", "var_ext_pro_dev" );
	myTableItemRemoveAll("rtu_var_ext_table");
	for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
		var _dt = xVarManageExtDataBase[n]['io'].dt;
		var _dtn = xVarManageExtDataBase[n]['io'].dtn;
		var _pt = xVarManageExtDataBase[n]['io'].pt;
		var _saddr = "----"; var _ext_addr = "--"; var _addr = "----";
		if(__proto_is_modbus(_dt, _pt)) {
			_saddr = xVarManageExtDataBase[n]['io'].sa;
			_ext_addr = xVarManageExtDataBase[n]['io'].ea;
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_dlt645(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
			_saddr = xVarManageExtDataBase[n]['io'].dltad;
		} else if(__proto_is_devself(_dt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_devself_mid(_dt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		} else if(__proto_is_dust(_dt, _pt)) {
			_addr = xVarManageExtDataBase[n]['io'].ad;
		}
		varExtTableAddItem( 
			xVarManageExtDataBase[n].en, 
			xVarManageExtDataBase[n].na, 
			xVarManageExtDataBase[n].al, 
			varExtGetVarTypeName(xVarManageExtDataBase[n]['io'].vt,xVarManageExtDataBase[n]['io'].vs), 
			xVarManageExtDataBase[n]['io'].va, 
			inProtoDevList(xProtoDevList, _dt, _dtn, _pt) ? varExtGetProtoName(_dt, _dtn, _pt):"---", 
			_saddr, _ext_addr, _addr, 
			xVarManageExtDataBase[n]['storage'].se
		);
	}
	var table = window.document.getElementById("rtu_var_ext_table");
	onExtTableItemClick(table, table.rows[index]);
}

function getAllVarManageExtDataBase()
{
	MyGetJSONWithArg("正在获取采集变量表,请稍后...","/cgi-bin/getVarManageExtData?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			xVarManageExtDataBase = res.list.concat();
			xProtoDevList = res.protolist;
			refreshAllVarManageExtDataBase(0);
		} else {
			alert("获取失败,请重试");
		}
	});
}

function getAllVarManageExtDataVals()
{
	MyGetJSONWithArg("","/cgi-bin/getVarManageExtDataVals?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			var vals = res.list;
			var table = window.document.getElementById('rtu_var_ext_table');
			var rowNum = table.rows.length;
			for(var i=0;i<rowNum;i++) {
				table.rows[i].cells[5].innerHTML = vals[i];
			}
		}
	});
}

function getVarExtInfo()
{
	var obj = window.document.getElementById("var_ext_id");
	var id = -1;
	if( obj != null && obj.value.length > 0 ) {
		id = Number(obj.value);
	}
	
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		var setval = { n:Number(id) };
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/getVarManageExtData?", JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				if( xVarManageExtDataBase[id] != null ) {
					xVarManageExtDataBase[id] = res;
				}
				var table = window.document.getElementById("rtu_var_ext_table");
				onExtTableItemClick(table, table.rows[id]);
			} else {
				alert("获取失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行读取");
	}
}

function myTableItemRemoveAll(id)
{
	var table = window.document.getElementById(id);
	var rowNum = table.rows.length;
	if(rowNum > 0) {
		for(i=0;i<rowNum;i++) {
			table.deleteRow(i);
			rowNum = rowNum-1;
			i = i-1;
		}
	}
}

function onExtTableItemClick(tb,row)
{
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
		for (var i = 0; i < tb.rows.length; i++) {
			if( xVarManageExtDataBase[i].en > 0 ) {
				tb.rows[i].style.background="#FFFFFF";
			} else {
				tb.rows[i].style.background="#F0F0F0";
			}
		}
		row.style.background="#E5E5E5";
		
		var _rowIndex = row.rowIndex;//去掉表头
		setValue('var_ext_id', _rowIndex);//编号
		setCheckBoxEnable('var_ext_enable',xVarManageExtDataBase[_rowIndex].en);//启用
		
		//setValue('var_ext_enable', xVarManageExtDataBase[_rowIndex].en);
		setValue('var_ext_name0', xVarManageExtDataBase[_rowIndex].na);
		setValue('var_ext_name1', xVarManageExtDataBase[_rowIndex].na);
		setValue('var_ext_alias0', xVarManageExtDataBase[_rowIndex].al);
		setValue('var_ext_alias1', xVarManageExtDataBase[_rowIndex].al);
		//下拉框赋值方法
		//setSelectValueByClass('var_ext_vartype', xVarManageExtDataBase[_rowIndex]['io'].vt);
		setValue('var_ext_vartype0', xVarManageExtDataBase[_rowIndex]['io'].vt);
		setValue('var_ext_vartype1', xVarManageExtDataBase[_rowIndex]['io'].vt);
		//setValue('var_ext_vartype', xVarManageExtDataBase[_rowIndex]['io'].vt);
		setValue('var_ext_varsize', xVarManageExtDataBase[_rowIndex]['io'].vs);
		setValue('var_ext_vartype_rule', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vrl));
		setValue('var_ext_vmax', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vma));
		setValue('var_ext_vmin', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vmi));
		setValue('var_ext_vinit', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vii));
		setValue('var_ext_vratio', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].vrt));
		
		setCheckBoxEnable('var_ext_dev_rtu_self', PROTO_DEV_RTU_SELF == xVarManageExtDataBase[_rowIndex]['io'].dt || PROTO_DEV_RTU_SELF_MID == xVarManageExtDataBase[_rowIndex]['io'].dt);
		if(getChecked('var_ext_dev_rtu_self')) {
			if (PROTO_DEV_RTU_SELF == xVarManageExtDataBase[_rowIndex]['io'].dt) {
				setValue('var_ext_dev_rtu_self_type', '0');
			} else if (PROTO_DEV_RTU_SELF_MID == xVarManageExtDataBase[_rowIndex]['io'].dt) {
				setValue('var_ext_dev_rtu_self_type', '1');
			}
			setValue('var_ext_dev_rtu_self_list', xVarManageExtDataBase[_rowIndex]['io'].dtn);
			ext_oncheck_self_dev();
		}
		setValue('var_ext_devtype', xVarManageExtDataBase[_rowIndex]['io'].dt + "|" + xVarManageExtDataBase[_rowIndex]['io'].dtn + "|" + xVarManageExtDataBase[_rowIndex]['io'].pt);
		setValue('var_ext_pro_dev', xVarManageExtDataBase[_rowIndex]['io'].dt + "|" + xVarManageExtDataBase[_rowIndex]['io'].dtn);
		setValue('var_ext_pro_type', xVarManageExtDataBase[_rowIndex]['io'].pt);
		setValue('var_ext_addr', xVarManageExtDataBase[_rowIndex]['io'].ad);
		setValue('var_err_op', xVarManageExtDataBase[_rowIndex]['io'].eop);
		setValue('var_err_cnt', xVarManageExtDataBase[_rowIndex]['io'].ecnt);

		if(__proto_is_modbus(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_sync_faddr', _tostr_(xVarManageExtDataBase[_rowIndex]['io'].sfa));
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].sa);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].sa);
			setValue('var_modbus_op', xVarManageExtDataBase[_rowIndex]['io'].mbop);
			setValue('var_ext_extaddr', xVarManageExtDataBase[_rowIndex]['io'].ea);
			setValue('var_ext_extaddrofs', xVarManageExtDataBase[_rowIndex]['io'].ao);
		} else if(__proto_is_dlt645_2007(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_dlt546_op', '0x' + numToString(xVarManageExtDataBase[_rowIndex]['io'].dltop, 16, 8));
		} else if(__proto_is_dlt645_1997(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_slaveaddr0', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_slaveaddr1', xVarManageExtDataBase[_rowIndex]['io'].dltad);
			setValue('var_ext_dlt546_1997_op', '0x' + numToString(xVarManageExtDataBase[_rowIndex]['io'].dltop, 16, 4));
		} else if(__proto_is_dust(xVarManageExtDataBase[_rowIndex]['io'].dt, xVarManageExtDataBase[_rowIndex]['io'].pt)) {
			setValue('var_ext_dust_op', xVarManageExtDataBase[_rowIndex]['io'].dustop);
		}
		
		setValue('var_ext_varrw', xVarManageExtDataBase[_rowIndex]['io'].rw);
		setCheckBoxEnable('var_ext_storage_en0', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		setCheckBoxEnable('var_ext_storage_en1', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		//chen qq,有多个相同节点存在时，使用class赋值
		//setSelectValueByClass('var_ext_storage_en', xVarManageExtDataBase[_rowIndex]['storage'].se);//数据存盘
		setValue('var_ext_storage_step', xVarManageExtDataBase[_rowIndex]['storage'].ss);//存盘间隔
		setValue('var_ext_exp', xVarManageExtDataBase[_rowIndex]['io']['exp']);

		onVarExtVartypeChange( window.document.getElementById("var_ext_vartype0"), false );
	}
}

function onExtTableItemDbClick(tb,row)
{
	onExtTableItemClick(tb,row);
	showDialog('data_dialog');
	ext_onchange_proto(false);
	ext_oncheck_self_dev();
	onExtTableItemClick(tb,row);
}

function varExtGetRW(rw)
{
	switch(Number(rw)) {
	case 0: return "只读";
	case 1: return "只写";
	case 2: return "读写";
	}
	
	return "未知";
}

function varExtGetProtoDev(dev,n)
{
	var str = "";
	dev = Number(dev);
	n = Number(n);
	switch(dev) {
	case PROTO_DEV_RS1: case PROTO_DEV_RS2: case PROTO_DEV_RS3: case PROTO_DEV_RS4: {
		return "COM"+(dev+1);
	}
	case PROTO_DEV_NET: {
		return "Net" + (n+1);
	}
	case PROTO_DEV_ZIGBEE: {
		return "Zigbee";
	}
	case PROTO_DEV_GPRS: {
		return GPRS_OR_NBIOT + (n+1);
	}
	case PROTO_DEV_RTU_SELF: {
		if(n<8) {
			return "RTU_A" + (n+1);
		}
	}
	case PROTO_DEV_RTU_SELF_MID: {
		return "中间变量";
	}
	}
	
	return "";
}

function varExtGetProto(dev,proto)
{
	var str = "";
	dev = Number(dev);
	proto = Number(proto);
	switch(dev) {
	case PROTO_DEV_RS1: case PROTO_DEV_RS2: case PROTO_DEV_RS3: case PROTO_DEV_RS4: {
		switch(proto) {
		case PROTO_MODBUS_RTU: return "Modbus_RTU";
		case PROTO_MODBUS_ASCII: return "Modbus_ASCII";
		case PROTO_DLT645: return "DLT645-2007";
		case PROTO_DLT645_1997: return "DLT645-1997";
		case PROTO_DUST: return "粉尘浓度测量仪协议";
		case PROTO_LUA: return "Lua协议";
		}
		break;
	}
	case PROTO_DEV_NET: {
		switch(proto) {
		case PROTO_MODBUS_TCP: return "Modbus_TCP";
		case PROTO_CC_BJDC: return "大型公建通讯协议";
		case PROTO_MODBUS_RTU_OVER_TCP: return "Modbus_RTU_Over_TCP";
		case PROTO_HJT212: return "HJ/T212";
		case PROTO_DM101: return "DM101";
		}
		break;
	}
	case PROTO_DEV_ZIGBEE: {
		switch(proto) {
		case PROTO_MODBUS_RTU: return "Modbus_RTU";
		case PROTO_MODBUS_ASCII: return "Modbus_ASCII";
		}
		break;
	}
	case PROTO_DEV_GPRS: {
		switch(proto) {
		case PROTO_MODBUS_TCP: return "Modbus_TCP";
		case PROTO_MODBUS_RTU_OVER_TCP: return "Modbus_RTU_Over_TCP";
		case PROTO_HJT212: return "HJ/T212";
		case PROTO_DM101: return "DM101";
		}
		break;
	}
	}
	
	return "";
}

function varExtGetProtoName(dev,n,proto)
{
	if(Number(dev) != PROTO_DEV_RTU_SELF && Number(dev) != PROTO_DEV_RTU_SELF_MID) {
		var str = varExtGetProtoDev(dev,n) + "_" +  varExtGetProto(dev,proto);
		if(str.length > 0) return str;
	} else {
		var str = varExtGetProtoDev(dev,n);
		if(str.length > 0) return str;
	}
	return "ERROR";
}

function varExtGetUpProtoName(dev,n,proto)
{
	if(Number(dev) != PROTO_DEV_RTU_SELF && Number(dev) != PROTO_DEV_RTU_SELF_MID) {
		var str = varExtGetProtoDev(dev, dev == PROTO_DEV_GPRS ? n - ENET_TCPIP_NUM : n) + "_" +  varExtGetProto(dev,proto);
		if(str.length > 0) return str;
	} else {
		var str = varExtGetProtoDev(dev,n);
		if(str.length > 0) return str;
	}
	return "ERROR";
}

function varExtGetVarTypeName(type,len)
{
	switch(Number(type)){
	case 0: return "BIT";
	case 1: return "INT8";
	case 2: return "UINT8";
	case 3: return "INT16";
	case 4: return "UINT16";
	case 5: return "INT32";
	case 6: return "UINT32";
	case 7: return "FLOAT";
	case 8: return "DOUBLE";
	case 9: return "ARRAY("+len+")";
	default: return "ERROR";
	}
}

function varExtTableAddItem(enable,name,alias,valtype,val,protodev,slaveaddr,extaddr,addr,storage)
{
	var table = window.document.getElementById("rtu_var_ext_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	row.onclick = function(){ onExtTableItemClick( table, row ); };
	row.ondblclick = function(){ onExtTableItemDbClick( table, row ); };
	obj = row.insertCell(0);
	obj.innerHTML = enable!=0?"启用":"未启用";
	var obj = row.insertCell(1);
	obj.innerHTML = table.rows.length-1;
	obj = row.insertCell(2);
	obj.innerHTML = name;
	obj = row.insertCell(3);
	obj.innerHTML = alias;
	obj = row.insertCell(4);
	obj.innerHTML = valtype;
	obj = row.insertCell(5);
	obj.innerHTML = val;
	obj = row.insertCell(6);
	obj.innerHTML = enable!=0?protodev:'--';
	obj = row.insertCell(7);
	obj.innerHTML = slaveaddr;
	obj = row.insertCell(8);
	obj.innerHTML = extaddr;
	obj = row.insertCell(9);
	obj.innerHTML = addr;
	obj = row.insertCell(10);
	obj.innerHTML = Number(storage)!=0?"是":"否";
}

function refresh_type_rule(_id,_n)
{
	var _rule = getValue(_id);
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	selectobj.options.length = 0;
	setDisplay(_id, true);
	if( _n < 5 || _n == 9 || getChecked('var_ext_dev_rtu_self')) {
		setDisplay(_id, false);
		selectobj.value = 0; 
	} else if( _n < 8 ) {
		selectobj.options.add(new Option('AB CD', '0'));
		selectobj.options.add(new Option('CD AB', '1'));
		selectobj.options.add(new Option('BA DC', '2'));
		selectobj.options.add(new Option('DC BA', '3'));
	} else if( _n == 8 ) {
		selectobj.options.add(new Option('AB CD EF GH', '0'));
		selectobj.options.add(new Option('GH EF CD AB', '1'));
		selectobj.options.add(new Option('BA DC FE HG', '2'));
		selectobj.options.add(new Option('HG FE DC BA', '3'));
	}
	selectobj.value = ''+_rule;
}

function onVarExtVartypeChange(obj,_change)
{
	if( obj.selectedIndex < 3 ) {
		setValue("var_ext_varsize", 1 );
	} else if( obj.selectedIndex < 5 ) {
		setValue("var_ext_varsize", 2 );
	} else if( obj.selectedIndex < 8 ) {
		setValue("var_ext_varsize", 4 );
	} else if( obj.selectedIndex == 8 ) {
		setValue("var_ext_varsize", 8 );
	}
	
	if( obj.selectedIndex < 9 ) {
		setEnable("var_ext_varsize", false );
	} else {
		setEnable("var_ext_varsize", true );
	}
	refresh_type_rule('var_ext_vartype_rule', obj.selectedIndex);
	if(_change) setValue("var_ext_vartype_rule", '0' );
}

function applyVarExtInfo(a,b)
{
	setValue('var_ext_name'+a, getValue('var_ext_name'+b));
	setValue('var_ext_alias'+a, getValue('var_ext_alias'+b));
	setValue('var_ext_vartype'+a, getValue('var_ext_vartype'+b));
	setValue('var_ext_slaveaddr'+a, getValue('var_ext_slaveaddr'+b));
	setCheckBoxEnable('var_ext_storage_en'+a, getChecked('var_ext_storage_en'+b));
	onVarExtVartypeChange( window.document.getElementById("var_ext_vartype0"), false );
	if( Number(a)==0 ) {
		if(getChecked('var_ext_dev_rtu_self')) {
			setValue('var_ext_devtype', '100|'+getNumber('var_ext_dev_rtu_self_list')+'|0');
		} else {
			setValue('var_ext_devtype', getValue('var_ext_pro_dev')+'|'+getValue('var_ext_pro_type'));
		}
	}
}

function getVarExtMapAddr()
{
	var _max_addr = 0;
	if( xVarManageExtDataBase != null ) {
		for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
			var _io = xVarManageExtDataBase[n]['io'];
			if( _io != null && _io.ad != null && _io.vs != null ) {
				if( Number(_io.ad) >= _max_addr ) {
					_max_addr = Number(_io.ad)+(Number(_io.vs)/2);
				}
			}
		}
	}
	return (_max_addr<1024?1024:_max_addr);
}

function addVarExtInfo()
{
	if( xVarManageExtDataBase != null ) {
		setValue('var_ext_id', xVarManageExtDataBase.length );
	} else {
		setValue('var_ext_id', "0" );
	}
	var _id = getNumber('var_ext_id');
	setCheckBoxEnable('var_ext_enable',1);
	setValue('var_ext_name0', "VR"+(1+_id));
	setValue('var_ext_name1', "VR"+(1+_id));
	setValue('var_ext_alias0', "");
	setValue('var_ext_alias1', "");
	setValue('var_ext_vartype0', 4);
	setValue('var_ext_vartype1', 4);
	setValue('var_ext_varsize', 2);
	
	setValue('var_ext_vmax', "");
	setValue('var_ext_vmin', "");
	setValue('var_ext_vinit', "");
	setValue('var_ext_vratio', "");
	
	setValue('var_ext_devtype', "");
	setValue('var_ext_pro_dev', "");
	setValue('var_ext_pro_type', "");
	setValue('var_ext_sync_faddr', 0);
	setValue('var_err_op', 0);
	setValue('var_err_cnt', 0);
	
	setValue('var_ext_addr', getVarExtMapAddr());
	setValue('var_ext_slaveaddr0', 5);
	setValue('var_ext_slaveaddr1', 5);
	setValue('var_modbus_op', 3);
	setValue('var_ext_extaddr', 0);
	setValue('var_ext_extaddrofs', 0);
	setValue('var_ext_varrw', 0);
	setCheckBoxEnable('var_ext_storage_en0', 0);
	setCheckBoxEnable('var_ext_storage_en1', 0);
	setValue('var_ext_storage_step', 5);
	setValue('var_ext_exp', "");
}

function checkVarExtName(_name, _n)
{
	if( xVarManageExtDataBase == null || _n >= xVarManageExtDataBase.length ) {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && xVarManageExtDataBase[n].na == _name ) {
				return n;
			}
		}
	} else {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && n != _n && xVarManageExtDataBase[n].na == _name ) {
				return n;
			}
		}
	}
	return -1;
}

function checkVarAddr(_addr, _n, _sz)
{
	var _a = _addr;
	var _b = _addr + _sz - 1;
	for(var n = 0; n < xVarManageExtDataBase.length; n++) {
		if (n != _n) {
			var _c = xVarManageExtDataBase[n]['io'].ad;
			var _d = xVarManageExtDataBase[n]['io'].ad + Math.round(xVarManageExtDataBase[n]['io'].vs / 2) - 1;
			if (_a == _c || _a ==_d || _b == _c || _b == _d) {
				return n;
			} else if (_a < _c && _c < _b && _b < _d) {
				return n;
			} else if (_a < _c && _d < _b) {
				return n;
			} else if (_c < _a && _a < _d && _d < _b) {
				return n;
			} else if (_c < _a && _b < _d) {
				return n;
			} 
		}
	}
	return -1;
}

function checkVarExtAlias(_alias, _n)
{
	if( xVarManageExtDataBase == null || _n >= xVarManageExtDataBase.length ) {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].na != null && xVarManageExtDataBase[n].al == _alias && _alias.length > 0 ) {
				return n;
			}
		}
	} else {
		for(var n = 0; n < xVarManageExtDataBase.length; n++) {
			if( xVarManageExtDataBase[n].al != null && n != _n && xVarManageExtDataBase[n].al == _alias && _alias.length > 0 ) {
				return n;
			}
		}
	}
	return -1;
}

function setVarExtInfo()
{
	var obj = window.document.getElementById("var_ext_id");
	var id = -1;
	if( obj != null && obj.value.length > 0 ) {
		id = Number(obj.value);
	}
		
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		
		onVarExtVartypeChange( window.document.getElementById("var_ext_vartype0"), false );
		var varsize = getNumber('var_ext_varsize')
		if( varsize > 32 ) {
			alert("最大长度为32字节,请修改后重新保存");
			return -1;
		}
		
		var addr = getNumber('var_ext_addr');

		if( addr < 1024 || addr > 10240 ) {
			alert("内部映射地址范围(1024->10240), 请修改后重新保存");
			return -1;
		}
		
		var err_cnt = getNumber('var_err_cnt');

		if( err_cnt > 255 ) {
			alert("错误次数范围(0->255), 请修改后重新保存");
			return -1;
		}
		
		var _var_ext_name = getValue('var_ext_name0');
		var _var_ext_alias = getValue('var_ext_alias0');
		if (chk(_var_ext_name, "采集变量名称")) return -1;
		
		if(getChecked('var_ext_dev_rtu_self')) {
			var self_type = getNumber('var_ext_dev_rtu_self_type');
			if (0 == self_type) {
				setValue('var_ext_vartype0', 7);
				setValue('var_ext_vartype1', 7);
				setValue('var_ext_varsize', 4);
			} else if (1 == self_type) {
				setValue('var_ext_vartype0', 8);
				setValue('var_ext_vartype1', 8);
				setValue('var_ext_varsize', 8);
			}
		}
		
		var proto = getValue("var_ext_devtype").split("|");
		var _dt = 0;
		if (getChecked('var_ext_dev_rtu_self')) {
			var self_type = getNumber('var_ext_dev_rtu_self_type');
			if (0 == self_type) {
				_dt = PROTO_DEV_RTU_SELF;
			} else if (1 == self_type) {
				_dt = PROTO_DEV_RTU_SELF_MID;
			}
		} else {
			_dt = Number(proto[0]);
		}
		var _io = {
			rw:getNumber('var_ext_varrw'),
			vt:getNumber('var_ext_vartype0'), 
			vs:getNumber('var_ext_varsize'), 
			vrl:getNumber('var_ext_vartype_rule'), 
			dt:_dt, 
			dtn:getChecked('var_ext_dev_rtu_self')?getNumber('var_ext_dev_rtu_self_list'):Number(proto[1]), 
			pt:getChecked('var_ext_dev_rtu_self')?0:Number(proto[2]), 
			ad:getNumber('var_ext_addr'),
			'exp':getValue('var_ext_exp'),
			'eop':getNumber('var_err_op'),
			'ecnt':getNumber('var_err_cnt')
		};
		
		if(__proto_is_modbus(_io.dt, _io.pt)) {
			
			var slaveaddr = getNumber('var_ext_slaveaddr0');
		
			if( slaveaddr < 0 || slaveaddr > 255 ) {
				alert("Modbus从机地址范围(0->255), 请修改后重新保存");
				return -1;
			}
			
			_io['sa'] = slaveaddr;
			_io['ea'] = getNumber('var_ext_extaddr');
			_io['mbop'] = getNumber('var_modbus_op');
			_io['ao'] = getNumber('var_ext_extaddrofs');
			_io['sfa'] = getNumber('var_ext_sync_faddr');
			
			var _var_ext_vmax = getValue('var_ext_vmax');
			var _var_ext_vmin = getValue('var_ext_vmin');
			var _var_ext_vinit = getValue('var_ext_vinit');
			var _var_ext_vratio = getValue('var_ext_vratio');
			if( _var_ext_vmax != null && _var_ext_vmax.length > 0 ) {
				_io['vma'] = Number(_var_ext_vmax);
			}
			if( _var_ext_vmin != null && _var_ext_vmin.length > 0 ) {
				_io['vmi'] = Number(_var_ext_vmin);
			}
			if( _var_ext_vinit != null && _var_ext_vinit.length > 0 ) {
				_io['vii'] = Number(_var_ext_vinit);
			}
			if( _var_ext_vratio != null && _var_ext_vratio.length > 0 ) {
				_io['vrt'] = Number(_var_ext_vratio);
			}
			
		} else if(__proto_is_dlt645_2007(_io.dt, _io.pt)) {
			var dltaddr = getValue('var_ext_slaveaddr0');
		
			if( dltaddr.length != 12 ) {
				alert("DLT645地址长度为12, 请修改后重新保存");
				return -1;
			}
			
			_io['dltad'] = dltaddr;
			_io['dltop'] = parseInt(getValue('var_ext_dlt546_op'),16);
		} else if(__proto_is_dlt645_1997(_io.dt, _io.pt)) {
			var dltaddr = getValue('var_ext_slaveaddr0');
		
			if( dltaddr.length != 12 ) {
				alert("DLT645地址长度为12, 请修改后重新保存");
				return -1;
			}
			
			_io['dltad'] = dltaddr;
			_io['dltop'] = parseInt(getValue('var_ext_dlt546_1997_op'),16);
		} else if(__proto_is_dust(_io.dt, _io.pt)) {
			_io['dustop'] = getNumber('var_ext_dust_op');
		}
		
		//var _alarm = {
		//	en:getNumber('var_ext_enable'), 
		//};
		var _storage = {
			se:getChecked('var_ext_storage_en0')?1:0,
			ss:getNumber('var_ext_storage_step')
		};
		var setval = {
			n:id, 
			en:getChecked('var_ext_enable')?1:0, 
			na:getValue('var_ext_name0'), 
			al:getValue('var_ext_alias0'), 
			'io':_io, 
			//'alarm':_alarm, 
			'storage':_storage
		};
		
		if( checkVarExtName(setval.na, id) >= 0 ) {
			alert("变量名重复，请检查！");
			return -1;
		}
		if( checkVarExtAlias(setval.al, id) >= 0 ) {
			alert("别名重复，请检查！");
			return -1;
		}
		
		var __index = checkVarAddr(addr, id, Math.round(_io.vs / 2));
		if( __index >= 0 ) {
			alert("映射地址已被变量["+ xVarManageExtDataBase[__index].na +"]使用，请检查！\r\n**注意：[" + $('#var_ext_vartype0 option:selected').text() + "]占用" + Math.round(_io.vs / 2) + "个寄存器**");
			return -1;
		}
		
		if( xVarManageExtDataBase != null && id < xVarManageExtDataBase.length ) {
			xVarManageExtDataBase[id].en=setval.en;
			xVarManageExtDataBase[id].na=setval.na;
			xVarManageExtDataBase[id].al=setval.al;
			
			if(xVarManageExtDataBase[id]['io']==null) xVarManageExtDataBase[id]['io'] = new Array();
			xVarManageExtDataBase[id]['io'].rw=_io.rw;
			xVarManageExtDataBase[id]['io'].vt=_io.vt;
			xVarManageExtDataBase[id]['io'].vs=_io.vs;
			xVarManageExtDataBase[id]['io'].dt=_io.dt;
			xVarManageExtDataBase[id]['io'].dtn=_io.dtn;
			xVarManageExtDataBase[id]['io'].pt=_io.pt;
			xVarManageExtDataBase[id]['io'].ad=_io.ad;
			if(__proto_is_modbus(_io.dt, _io.pt)) {
				xVarManageExtDataBase[id]['io'].sa=_io.sa;
				xVarManageExtDataBase[id]['io'].ea=_io.ea;
				xVarManageExtDataBase[id]['io'].ao=_io.ao;
				xVarManageExtDataBase[id]['io'].sfa=_io.sfa;
			} else if(__proto_is_dlt645(_io.dt, _io.pt)) {
				xVarManageExtDataBase[id]['io'].dltad=_io.dltad;
				xVarManageExtDataBase[id]['io'].dltop=_io.dltop;
			}
			xVarManageExtDataBase[id]['io'].exp=_io.exp;
			
			if( _io.vma != null) xVarManageExtDataBase[id]['io'].vma=_io.vma;
			if( _io.vmi != null) xVarManageExtDataBase[id]['io'].vmi=_io.vmi;
			if( _io.vii != null) xVarManageExtDataBase[id]['io'].vii=_io.vii;
			if( _io.vrt != null) xVarManageExtDataBase[id]['io'].vrt=_io.vrt;
			
			xVarManageExtDataBase[id]['storage'].se=_storage.se;
			xVarManageExtDataBase[id]['storage'].ss=_storage.ss;
		}
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/setVarManageExtData?", JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				getAllVarManageExtDataBase();
				refreshAllVarManageExtDataBase(id);
				alert( "设置成功" );
			} else {
				alert("设置失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行设置");
	}
	
	return 0;
}

function ext_onchange_proto(_resize)
{
	var _show_div_id = 'ext_dlt546_1997_div';
	setDisplay('ext_modbus_div', false);
	setDisplay('ext_dlt546_2007_div', false);
	setDisplay('ext_dlt546_1997_div', false);
	setDisplay('ext_dust_div', false);
	setDisplay(_show_div_id, true);
	var protoDevList = window.document.getElementById('var_ext_pro_dev');
	var protoTypeList = window.document.getElementById('var_ext_pro_type');
	if(protoTypeList.options.length > 0 && protoDevList.options.length > 0 ) {
		var devary = protoDevList.value.split("|");
		if(devary.length==2) {
			var _dev = Number(devary[0]);
			var _po = Number(protoTypeList.value);
			if(__proto_is_modbus(_dev,_po)) {
				setDisplay('ext_modbus_div', true);
				if(_show_div_id != 'ext_modbus_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 4);
					setValue('var_ext_vartype1', 4);
					setValue('var_ext_varsize', 2);
					setEnable('var_ext_varsize', false);
				}
			} else if(__proto_is_dlt645_2007(_dev,_po)) {
				setDisplay('ext_dlt546_2007_div', true);
				if(_show_div_id != 'ext_dlt546_2007_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_varsize', 4);
					setEnable('var_ext_varsize', false);
				}
			} else if(__proto_is_dlt645_1997(_dev,_po)) {
				setDisplay('ext_dlt546_1997_div', true);
				if(_show_div_id != 'ext_dlt546_1997_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_varsize', 4);
					setEnable('var_ext_varsize', false);
				}
			} else if(__proto_is_dust(_dev,_po)) {
				setDisplay('ext_dust_div', true);
				if(_show_div_id != 'ext_dust_div') setDisplay(_show_div_id, false);
				if(_resize) {
					setValue('var_ext_vartype0', 7);
					setValue('var_ext_vartype1', 7);
					setValue('var_ext_varsize', 4);
					setEnable('var_ext_varsize', false);
				}
			}
		}
	}
}

function ext_oncheck_self_dev()
{
	setDisplay('ext_err_op_div', true);
	if(getChecked('var_ext_dev_rtu_self')) {
		setDisplay('ext_modbus_div', false);
		setDisplay('ext_dlt546_2007_div', false);
		setDisplay('ext_dlt546_1997_div', false);
		setDisplay('ext_dust_div', false);
		setDisplay('ext_not_rtu_self_div', false);
		setDisplay('ext_not_rtu_self_div1', false);
		setDisplay('var_ext_dev_rtu_self_list_ul', true);
		setValue('var_ext_vartype0', 7);
		setValue('var_ext_vartype1', 7);
		setValue('var_ext_varsize', 4);
		setEnable('var_ext_varsize', false);
		setEnable('var_ext_vartype0', false);
		setEnable('var_ext_vartype1', false);
		ext_onchange_dev_rtu_self_type();
	} else {
		setDisplay('var_ext_dev_rtu_self_list_ul', false);
		setDisplay('ext_not_rtu_self_div', true);
		setDisplay('ext_not_rtu_self_div1', true);
		setEnable('var_ext_vartype0', true);
		setEnable('var_ext_vartype1', true);
		refreshOneProtoDevList(false,'var_ext_pro_dev','var_ext_pro_type');
		ext_onchange_proto(true);
	}
}

function ext_onchange_dev_rtu_self_type()
{
	var self_type = getNumber('var_ext_dev_rtu_self_type');
	setDisplay('var_ext_dev_rtu_self_list', false);
	setDisplay('ext_err_op_div', true);
	if(self_type == 0) {
		setDisplay('var_ext_dev_rtu_self_list', true);
	} else if(self_type == 1) {
		setDisplay('ext_err_op_div', false);
		setValue('var_ext_vartype0', 8);
		setValue('var_ext_vartype1', 8);
		setValue('var_ext_varsize', 8);
	}
}

// 该函数关联性比较强
function refreshOneProtoDevList(isup,prodevid,protypeid)
{
	var protoTypeList = window.document.getElementById(protypeid);
	var n = 0;
	var _dev = getValue(prodevid).split("|");
	if( _dev.length == 2 && isup != null ) {
		var dev = Number(_dev[0]);
		var dev_n = Number(_dev[1]);
		var res = isup?xUpProtoDevList:xProtoDevList;
		protoTypeList.options.length = 0;
		if( res.rs != null ) {
			for( var n = 0; n < res.rs.length; n++ ) {
				if( dev == res.rs[n].id) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.rs[n].id, res.rs[n].po), ""+res.rs[n].po ) 
					);
				}
			}
		}
		if( res.net != null ) {
			for( var n = 0; n < res.net.length; n++ ) {
				if( dev == res.net[n].id && dev_n == res.net[n].idn) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.net[n].id, res.net[n].po), ""+res.net[n].po ) 
					);
				}
			}
		}
		if( res.zigbee != null ) {
			for( var n = 0; n < res.zigbee.length; n++ ) {
				if( dev == res.zigbee[n].id ) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.zigbee[n].id, res.zigbee[n].po), ""+res.zigbee[n].po ) 
					);
				}
			}
		}
		if( res.gprs != null ) {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( dev == res.gprs[n].id && dev_n == res.gprs[n].idn ) {
					protoTypeList.options.add( 
						new Option( varExtGetProto(res.gprs[n].id, res.gprs[n].po), ""+res.gprs[n].po ) 
					);
				}
			}
		}
	}
}

function inProtoDevList(res, _id, _idn, _po)
{
	if(PROTO_DEV_RTU_SELF==Number(_id)) return true;
	if(PROTO_DEV_RTU_SELF_MID==Number(_id)) return true;
	if( res.rs != null ) {
		for( var n = 0; n < res.rs.length; n++ ) {
			if(res.rs[n].id == _id && res.rs[n].po == _po) return true;
		}
	}
	if( res.net != null ) {
		for( var n = 0; n < res.net.length; n++ ) {
			if(res.net[n].id == _id && res.net[n].idn == _idn && res.net[n].po == _po) return true;
		}
	}
	if( res.zigbee != null ) {
		for( var n = 0; n < res.zigbee.length; n++ ) {
			if(res.zigbee[n].id == _id && res.zigbee[n].po == _po) return true;
		}
	}
	if( res.gprs != null ) {
		for( var n = 0; n < res.gprs.length; n++ ) {
			if(res.gprs[n].id == _id && res.gprs[n].po == _po) return true;
		}
	}
}

function refreshProtoDevList(res, devtypeid, prodevid)
{
	var protoDevList = window.document.getElementById(devtypeid);
	var devNameList = window.document.getElementById(prodevid);
	if( protoDevList != null ) protoDevList.options.length = 0;
	if( devNameList != null ) devNameList.options.length = 0;
	if( res.rs != null ) {
		for( var n = 0; n < 8; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( PROTO_DEV_RTU_SELF, n, 0 ), 
						PROTO_DEV_RTU_SELF + "|" + n + "|" + 0
					)
				);
			}
		}
		if( protoDevList != null ) {
			protoDevList.options.add(
				new Option(
					varExtGetProtoName( PROTO_DEV_RTU_SELF_MID, 0, 0 ), 
					PROTO_DEV_RTU_SELF_MID + "|0|0"
				)
			);
		}
		for( var n = 0; n < res.rs.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.rs[n].id, res.rs[n].idn, res.rs[n].po ), 
						res.rs[n].id + "|" + res.rs[n].idn + "|" + res.rs[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.rs[n].id, res.rs[n].idn ), 
						res.rs[n].id + "|" + res.rs[n].idn
					)
				);
			}
		}
	}
	if( res.net != null ) {
		for( var n = 0; n < res.net.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.net[n].id, res.net[n].idn, res.net[n].po ), 
						res.net[n].id + "|" + res.net[n].idn + "|" + res.net[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.net[n].id, res.net[n].idn ), 
						res.net[n].id + "|" + res.net[n].idn
					)
				);
			}
		}
	}
	if( res.zigbee != null ) {
		for( var n = 0; n < res.zigbee.length; n++ ) {
			if( protoDevList != null ) {
				protoDevList.options.add(
					new Option(
						varExtGetProtoName( res.zigbee[n].id, res.zigbee[n].idn, res.zigbee[n].po), 
						res.zigbee[n].id + "|" + res.zigbee[n].idn + "|" + res.zigbee[n].po
					)
				);
			}
			if( devNameList != null ) {
				devNameList.options.add(
					new Option(
						varExtGetProtoDev( res.zigbee[n].id, res.zigbee[n].idn ), 
						res.zigbee[n].id + "|" + res.zigbee[n].idn
					)
				);
			}
		}
	}
	if( res.gprs != null ) {
		
		if ("upload_data_pro_dev" == prodevid) {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( protoDevList != null ) {
					protoDevList.options.add(
						new Option(
							varExtGetProtoName( res.gprs[n].id, res.gprs[n].idn - ENET_TCPIP_NUM, res.gprs[n].po), 
							res.gprs[n].id + "|" + res.gprs[n].idn + "|" + res.gprs[n].po
						)
					);
				}
				if( devNameList != null ) {
					devNameList.options.add(
						new Option(
							varExtGetProtoDev( res.gprs[n].id, res.gprs[n].idn - ENET_TCPIP_NUM) , 
							res.gprs[n].id + "|" + res.gprs[n].idn
						)
					);
				}
			}
		} else {
			for( var n = 0; n < res.gprs.length; n++ ) {
				if( protoDevList != null ) {
					protoDevList.options.add(
						new Option(
							varExtGetProtoName( res.gprs[n].id, res.gprs[n].idn, res.gprs[n].po), 
							res.gprs[n].id + "|" + res.gprs[n].idn + "|" + res.gprs[n].po
						)
					);
				}
				if( devNameList != null ) {
					devNameList.options.add(
						new Option(
							varExtGetProtoDev( res.gprs[n].id, res.gprs[n].idn ), 
							res.gprs[n].id + "|" + res.gprs[n].idn
						)
					);
				}
			}
		}
	}
}

function refreshDevInfo(info)
{
	setValue('rtu_dev_id', info.id);
	setValue('rtu_dev_hw_ver', info.hw);
	setValue('rtu_dev_sw_ver', info.sw);
	setValue('rtu_dev_oem', info.om);
	setValue('rtu_dev_mac', info.mc);
	setValue('rtu_dev_time', info.dt);
	setValue('rtu_dev_desc', info.desc);
	setValue('rtu_das_module_desc', info.das_desc);
	
	setValue('rtu_dev_zigbee_ver', info.zgbver);
}

function getDevInfo()
{
	MyGetJSONWithArg("正在获取设备信息,请稍后...", "/cgi-bin/getDevInfo?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			refreshDevInfo( res );
		} else {
			alert("获取设备信息失败,请重试");
		}
	});
}

function devTimeSync()
{
	var myDate = new Date();
	var setval = {
		ye:myDate.getFullYear(), 
		mo:(myDate.getMonth()+1), 
		da:myDate.getDate(), 
		dh:myDate.getHours(), 
		hm:myDate.getMinutes(), 
		ms:myDate.getSeconds()
	};
	
	MyGetJSONWithArg("正在对时,请稍后...","/cgi-bin/setTime?", JSON.stringify(setval));
}

function refreshNetInfo(info)
{
	/*setValue('rtu_dev_id', info.id);
	setValue('rtu_dev_hw_ver', Number(Number(info.hw) / 100.0).toFixed(2));
	setValue('rtu_dev_sw_ver', Number(Number(info.sw) / 100.0).toFixed(2));
	setValue('rtu_dev_oem', info.om);
	setValue('rtu_dev_mac', info.mc);
	setValue('rtu_dev_time', info.dt);*/
	
	if( info.status != null ) {
		window.document.getElementById("lbl_netStatus").innerHTML=info.status;
	}
}

function getNetInfo()
{
	MyGetJSONWithArg("正在获取网络信息,请稍后...", "/cgi-bin/getNetCfg?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			refreshNetInfo( res );
		} else {
			alert("获取网络信息失败,请重试");
		}
	});
}

function getAllTcpipCfg()
{
	MyGetJSONWithArg("正在获取TCP/IP配置,请稍后...", "/cgi-bin/getTcpipCfg?", "{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			xTcpipCfgList = res.list.concat();
			for( var n = 0; n < res.list.length; n++ ) {
				refreshTcpipCfg( 
					n, 
					res.list[n]
				);
			}
		} else {
			alert("获取TCP/IP配置失败,请重试");
		}
	});
}

function getTcpipCfg(i)
{
	var setval = { n:Number(i) };

	MyGetJSONWithArg("正在获取TCP/IP配置,请稍后...", "/cgi-bin/getTcpipCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			refreshTcpipCfg( i, res );
		} else {
			alert("获取TCP/IP配置失败,请重试");
		}
	});
}

function refreshTcpipCfg(n,cfg)
{
	var prefix = "net_tcpip";
	if(n >= ENET_TCPIP_NUM) {
		prefix = "gprs_tcpip";
		n -= ENET_TCPIP_NUM;
	}
	
	setValue(prefix+n+'_enable', cfg.en);
	setValue(prefix+n+'_type', cfg.tt);
	setValue(prefix+n+'_cs', cfg.cs);
	setValue(prefix+n+'_proto', cfg.pt);
	setValue(prefix+n+'_ms', cfg.ms);
	setValue(prefix+n+'_peer', cfg.pe);
	setValue(prefix+n+'_port', cfg.po);
}

function setTcpipCfg(i)
{	
	var n = i;
	var prefix = "net_tcpip";
	if(i >= ENET_TCPIP_NUM) {
		prefix = "gprs_tcpip";
		i -= ENET_TCPIP_NUM;
	}
	var setval = {
		n:Number(n), 
		en:getNumber(prefix + i + '_enable'), 
		tt:getNumber(prefix + i + '_type'), 
		pt:getNumber(prefix + i + '_proto'), 
		cs:getNumber(prefix + i + '_cs'), 
		ms:getNumber(prefix + i + '_ms'), 
		pe:getValue(prefix + i + '_peer'), 
		po:getNumber(prefix + i + '_port')
	};

	MyGetJSONWithArg("正在配置TCP/IP,请稍后...", "/cgi-bin/setTcpipCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			alert("配置TCP/IP成功");
		} else {
			alert("配置TCP/IP失败,请重试");
		}
	});
}


function getDOValue(i)
{
	if( i < 4 ) {
		return getValue('btn_writeDO_'+i) == "  开  "?1:0;
	} else {
		return getNumber('dido_do_' + i);
	}
}

function setDOValue(i, val)
{
	if( i < 4 ) {
		setValue('btn_writeDO_'+i, val!=0?"  关  ":"  开  ");
	} else {
		setValue( "dido_do_"+i, val );
	}
}

function readDIDO()
{
	MyGetJSONWithArg("正在读取输入输出模块信息,请稍后...", "/cgi-bin/readDIDO?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			for( var i = 0; i < res.di.length; i++ ) {
				setValue( "dido_di_"+i, res.di[i].va );
			}
			for( var i = 0; i < res['do'].length; i++ ) {
				setDOValue( i, res['do'][i].va );
			}
		} else {
			alert("获取失败,请重试");
		}
	});
}

function writeDO(i)
{
	var setval = {
		n:Number(i), 
		va:getDOValue(i)
	};
	MyGetJSONWithArg("正在设置输出模块信息,请稍后...","/cgi-bin/writeDO", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			for( var i = 0; i < res.di.length; i++ ) {
				setValue( "dido_di_"+i, res.di[i].va );
			}
			for( var i = 0; i < res['do'].length; i++ ) {
				setDOValue( i, res['do'][i].va );
			}
			alert("设置成功");
		} else {
			alert("设置失败,请重试");
		}
	});
}

function onAiTypeChange(n)
{
	setVisible("ai_extval_set_"+n, getNumber("ai_type_"+n)==3);
}

function setAICfg(n)
{
	var setval = {
		n:Number(n), 
		rg:getNumber('ai_range_' + n), 
		ut:getNumber('ai_type_' + n), 
		ei:getNumber('ai_extval_min_' + n), 
		ea:getNumber('ai_extval_max_' + n), 
		ec:getNumber('ai_extval_corr_' + n)
	};
	MyGetJSONWithArg("正在设置通道"+(n+1)+",请稍后...","/cgi-bin/setAnalogCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			alert("设置成功");
		} else {
			alert("设置失败,请重试");
		}
	});
}

function refreshAICfg(n,cfg)
{
	if( cfg != null ) {
		setValue( "ai_range_"+n, cfg.rg );
		setValue( "ai_type_"+n, cfg.ut );
		setValue( "ai_extval_min_"+n, cfg.ei );
		setValue( "ai_extval_max_"+n, cfg.ea );
		setValue( "ai_extval_corr_"+n, cfg.ec );
		setValue( "lbl_ai_val_"+n, cfg.va );
		var obj = window.document.getElementById("lbl_ai_val_"+n);
		if( obj != null && cfg.va != null ) {
			obj.innerHTML = "当前值:" + cfg.va;
		}
	}
	onAiTypeChange(n);
}

function getAllAICfg()
{
	MyGetJSONWithArg("正在读取模拟量配置,请稍后...","/cgi-bin/getAnalogCfg?","{\"all\":1}", function (res) {
		if( res != null && 0 == res.ret ) {
			for( var i = 0; i < res.list.length; i++ ) {
				refreshAICfg( i, res.list[i] );
			}
		} else {
			alert("获取失败,请重试");
		}
	});
}

function refreshGPRSInfo(info)
{
	setValue( "gprs_info_apn", info.apn );
	setValue( "gprs_info_user", info.user );
	setValue( "gprs_info_psk", info.psk );
	
	if( info.status != null ) {
		window.document.getElementById("lbl_GPRSState").innerHTML=info.status;
	}
}

function getGPRSInfo()
{
	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "模块,请稍后...","/cgi-bin/getGPRSCfg?", "", function (res) {
		if( res != null && 0 == res.ret ) {
			refreshGPRSInfo(res);
		} else {
			alert("读取失败,请重试");
		}
	});
}

function setGPRSInfo()
{
	var setval = {
		apn:getValue("gprs_info_apn"), 
		user:getValue("gprs_info_user"), 
		psk:getValue("gprs_info_psk")
	};

	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "模块,请稍后...","/cgi-bin/setGPRSCfg?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			alert( "设置成功" );
		} else {
			alert("设置失败,请重试");
		}
	});
}


//======================================================页面js函数==================================================
//每次选择菜单时，都会填写【功能名称】，待调用ak47时使用；
function bullet(name)
{
	document.getElementById("txt_refresh_name").value = name;
}

function bullet_clear()
{
	document.getElementById("txt_refresh_name").value = "";
}

function bullet_add(name) 
{
	document.getElementById("txt_refresh_name").value = name;
}


function msg()
{
	alert('应用成功！');
}

function chk(obj,name)
{
	if (obj == "" || obj == "undefined")
	{
		alert("【" + name + "】不能为空！");
		return true;
	}
	else
	{
		return false;
	}
}

function chk2(obj1, obj2, name)
{
	if ((obj1 == "" || obj1 == "undefined") && (obj2 == "" || obj2 == "undefined"))
	{
		alert("【" + name + "】不能为空！");
		return true;
	}
	else
	{
		return false;
	}
}

var v = 0;
var bakname = "";
//自动刷新时，调用该函数；
function ak47()
{
	var name = document.getElementById("txt_refresh_name").value;
	/*if( bakname != name ) {
		bakname = name;
		bullet_clear();
	}*/
	switch (name)
	{
		case "1001":
			{
				//设备基本信息
				Show_Basic_Information();
			}
			break;
		case "1002":
			{
				//系统资源
				Show_System_Resource();
			}
			break;
		case "1003":
			{
				Show_All_NetInfo();
			}
			break;
		case "1004":
			{
				//已连接Zigbee无线节点
				Show_Zgb_Information();
			}
			break;
		case "1005":
			{
				//基本设置
				Show_Time_Synchronization();
				Show_Storage_Cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1006":
			{
				//日志
				Show_Log();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1007":
			{
				//管理权限
				Show_Jurisdiction();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1008":
			{
				//远程传输设置
				Show_Remote_Transmission();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1009":
			{
				//本地网络配置
				Show_Local_Network_Configuration();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1010":
			{
				//ZIGBEE配置
				Show_ZIGBEE();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1012":
			{
				//工作模式及中心配置
				Show_GPRS_Transmission();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1013":
			{
				//GPRS工作参数配置
				Show_GPRS_Config();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1014":
			{
				//无线参数
				Show_GPRS_Wireless();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1015":
			{
				//串口配置
				Show_Serial_Port();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1016":
			{
				//开关量输入设置
				Show_Switch_Cfg();
				//跳转
				bullet_add("9016");
			}
			break;
		case "9016":
			{
				Show_Switch_Value();
			}
			break;
		case "1017":
			{
				//开关量输出设置
				Show_Output_Cfg();
				Show_Output_Value();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1018":
			{
				//模拟量输入设置
				Show_Input();
				//跳转
				bullet_add("9018");
			}
			break;

		case "9018":
			{
				Show_Input_Range_Electrical();
			}
			break;
		case "1021":
			{
				//采集变量
				getAllVarManageExtDataBase();
				//跳转
				bullet_add("9021");
			}
			break;
		case "9021":
			{
				getAllVarManageExtDataVals();
				break;
			}
		case "1022":
			{
				//用户变量
				Show_Defin();
				//仅刷新一次；
				bullet_clear();
			}
			break;

		case "1023":
			{
				//动作
			}
			break;
			
		case "1024":
			{
				Show_Xfer_Net_Cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1029":
			{
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1030"://上传数据配置
			{
				Show_upload_cfg();
				//仅刷新一次；
				bullet_clear();
			}
			break;
		case "1050"://网络适配器配置
			{
				Show_Network_Information();
				//仅刷新一次；
				bullet_clear();
			}
			break;
						
		default: break;
	}

}

function goreg()
{
	window.location.href="http://"+window.location.host+"/reg.html";
}

//设备基本信息
function Show_Basic_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getDevInfo?","", function (info)
	{
		Page_Basic_Information(info)
	});
}

function Page_Basic_Information( info )
{	
	if( info.reg == 1 ) {
		window.document.getElementById("rtu_reg_status").innerHTML = "已激活";
		setVisible('btn_goreg', false);
	} else {
		var t = info.tta-info.ttt;
		if( t > 0 ) {
			var d = parseInt(t/(24*60*60));
			t-=(24*60*60*d);
			var h = parseInt(t/(60*60));
			t-=60*60*h;
			var m = parseInt(t/60);
			window.document.getElementById("rtu_reg_status").innerHTML = "试用剩余时间 " + d + "天" + h + "时" + m + "分";
		} else {
			window.document.getElementById("rtu_reg_status").innerHTML = "试用结束，请激活！";
		}
		setVisible('btn_goreg', true);
	}
	//设备ID
	document.getElementById("rtu_dev_id").innerHTML = info.id;
	//设备序列号
	document.getElementById("rtu_dev_sn").innerHTML = info.sn;
	//硬件版本
	document.getElementById("rtu_dev_hw_ver").innerHTML = info.hw;
	//固件版本
	document.getElementById("rtu_dev_sw_ver").innerHTML = info.sw;
	//产品序列号
	document.getElementById("rtu_dev_oem").innerHTML = info.om;
	//Zigbee 固件版本
	document.getElementById("rtu_dev_zigbee_ver").innerHTML = info.zgbver;
	//MAC 地址
	document.getElementById("rtu_dev_mac").innerHTML = info.mc;
	//系统时间
	document.getElementById("rtu_dev_time").innerHTML = info.dt;
	//系统信息
	document.getElementById("rtu_dev_desc").innerHTML = info.desc;
	//模块信息
	document.getElementById("rtu_das_module_desc").innerHTML = info.das_desc;
	//运行时间
	document.getElementById("rtu_run_time").innerHTML = info.rt;
	
	var date = new Date(info.dt);
	var nowdate = new Date();
	var datediff = nowdate.getTime()-date.getTime();
	if( datediff > 10000 ) {
		devTimeSync();
	}
}

//系统资源
function Show_System_Resource()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_System_Resource("注：此处传入一个对象");
	MyGetJSONWithArg("","/cgi-bin/getCpuUsage?","", function (info)
	{
		Page_System_Resource(info)
	});
}

//cpu使用率和内存占用有% 可用进度条 内存剩余，存储ROW，SD卡没有进度条
function Page_System_Resource(info)
{
	//CPU使用率
	document.getElementById("sys_cpu_rate").innerHTML = Number(info.cpu) + "%";
	$("#sys_cpu_rate").css('width',Number(info.cpu) + "%");
	//内存占用
	if( Number(info.ms) > 0 ) {
		document.getElementById("sys_memory_use").innerHTML = Math.round(100*Number(info.mu)/Number(info.ms)) + "%";
		$("#sys_memory_use").css('width',Math.round(100*Number(info.mu)/Number(info.ms)) + "%");
	} else {
		document.getElementById("sys_memory_use").innerHTML = "--";
		$("#sys_memory_use").css('width',"0%");
	}
	//内存剩余
	document.getElementById("sys_memory_free").innerHTML = "Free : " + 
				((Number(info.ms)-Number(info.mu))/1024).toFixed(2) + " MB  [Total : " + 
				(Number(info.ms)/1024).toFixed(2) + " MB, Used : " +  
				(Number(info.mu)/1024).toFixed(2) + " MB, Max Used : " +  (Number(info.ma)/1024).toFixed(2) + " MB]";

	//存储ROM
	document.getElementById("sys_store_rom").innerHTML = ((Number(info.fs)-Number(info.fu))/1024).toFixed(2) + " MB";
	//SD卡
	document.getElementById("sys_store_sd").innerHTML = ((Number(info.ss)-Number(info.su)) / 1024).toFixed(2) + " MB";
}

//网络信息
function Show_Network_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getNetInfo?","", function (info)
	{
		Page_Network_Information(info)
	});
}

function Page_Network_Information(info)
{
	//类型: dhcp
	document.getElementById("sys_net_dhcp").innerHTML = Number(info.dh)!=0?"开":"关";
	//MAC地址
	document.getElementById("sys_net_mac").innerHTML = info.mac;
	//地址
	document.getElementById("sys_net_address").innerHTML = info.ip;
	//给调试监听添加IP地址
	$('#txtWebSocketAddress').val(info.ip);
	//子网掩码
	document.getElementById("sys_net_mask").innerHTML = info.mask;
	//网关
	document.getElementById("sys_net_gate").innerHTML = info.gw;
	//DNS 1
	document.getElementById("sys_net_dns_1").innerHTML = info.d1;
	//DNS 2
	document.getElementById("sys_net_dns_2").innerHTML = info.d2;
	//已连接
	document.getElementById("sys_net_link").innerHTML = info.lk;
	
	if (!info.link) {
		setDisplay('net_no_link', true);
		setEnable('net_adapter', false);
		setValue('net_adapter', '1');
		document.getElementById("sys_net_link").innerHTML = "网线未连接/异常";
	} else {
		setDisplay('net_no_link', false);
		setEnable('net_adapter', true);
		setValue('net_adapter', info.adpt);
	}
	document.getElementById("sys_net_info_title").innerHTML = "有线网络" + (info.adpt == NET_ADAPTER_WIRED ? "【当前网络】" : "");

}

//已连接Zigbee无线节点
function Show_Zgb_Information()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("","/cgi-bin/getZigbeeList?","", function (info)
	{
		Page_Zgb_Information(info)
	});
}

function zgbNodeMode(mode)
{
	if( mode != null ) {
		switch(Number(mode)) {
		case 0:
			return "终端";
		case 1:
			return "路由器";
		case 2:
			return "协调器";
		}
	}
	return "未知";
}

function zgbNodeTableAddItem(mode,mac,netid,on,rssi,adlst,upt,offt)
{
	var table = window.document.getElementById("zgb_node_table");
	var row = table.insertRow(table.rows.length);
	var obj = row.insertCell(0);
	obj.innerHTML = zgbNodeMode(mode);
	obj = row.insertCell(1);
	obj.innerHTML = mac;
	obj = row.insertCell(2);
	obj.innerHTML = netid;
	obj = row.insertCell(3);
	obj.innerHTML = Number(on)!=0?"在线":"下线";
	obj = row.insertCell(4);
	if(Number(on)!=0 && rssi!=null && Number(rssi)>0) {
		obj.innerHTML = Math.round(100*Number(rssi)/255) + "%";
	} else {
		obj.innerHTML = "未知";
	}
	obj = row.insertCell(5);
	obj.innerHTML = adlst;
	obj = row.insertCell(6);
	obj.innerHTML = upt;
	obj = row.insertCell(7);
	obj.innerHTML = offt;
}

function zgbNodeTableItemRemoveAll()
{
	var table = window.document.getElementById("zgb_node_table");
	var rowNum = table.rows.length;
	if(rowNum > 0) {
		for(i=0;i<rowNum;i++) {
			table.deleteRow(i);
			rowNum = rowNum-1;
			i = i-1;
		}
	}
}

function Page_Zgb_Information(info)
{
	if(info.list != null ) {
		zgbNodeTableItemRemoveAll();
		for( var n = 0; n < info.list.length; n++ ) {
			zgbNodeTableAddItem( 
				info.list[n].mode, 
				info.list[n].mac, 
				info.list[n].netid, 
				info.list[n].on, 
				info.list[n].rssi, 
				info.list[n].adlst, 
				info.list[n].upt, 
				info.list[n].offt
			);
		}
	}
}

//GPRS网络信息
function Show_gprs_network()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_gprs_network("")
	MyGetJSONWithArg("","/cgi-bin/getGPRSState?","", function (info)
	{
		Page_gprs_network(info);
	});
}

function Page_gprs_network(info)
{
	var reg = (info.creg!=null?Number(info.creg):-1);
	var _pwr = (info.pwr!=null?(Number(info.pwr)!=0):1);
	//开机状态
	document.getElementById("gprs_network_pwr").innerHTML = _pwr?"开机":"关机";
	if(_pwr) {
		//运营商名称
		document.getElementById("gprs_network_name").innerHTML = info.alphan;
		document.getElementById("gprs_network_type").innerHTML = (info.n_type == 0 ? "GPRS" : "LTE");
		document.getElementById("gprs_network_ip").innerHTML = info.ip;
		//网络注册状态
		if( reg <= 0 || reg == 2 || reg == 3 || reg == 4 ) {
			document.getElementById("gprs_network_creg").innerHTML = "未注册";
		} else if( reg == 1 ) {
			document.getElementById("gprs_network_creg").innerHTML = "本地网络";
		} else if( reg == 5 ) {
			document.getElementById("gprs_network_creg").innerHTML = "漫游网络";
		} else if( reg == 8 ) {
			document.getElementById("gprs_network_creg").innerHTML = "紧急呼叫";
		} else {
			document.getElementById("gprs_network_creg").innerHTML = "未知状态("+reg+")";
		}
		//信号质量
		document.getElementById("gprs_network_csq").innerHTML = info.csq;
		//小区信息
		document.getElementById("gprs_network_area").innerHTML = info.area;
		
		document.getElementById("gprs_net_info_title").innerHTML = GPRS_OR_NBIOT + "网络" + (info.adpt == NET_ADAPTER_WIRELESS ? "【当前网络】" : "");
	} else {
		document.getElementById("gprs_network_name").innerHTML = "--";
		document.getElementById("gprs_network_type").innerHTML = "--";
		document.getElementById("gprs_network_ip").innerHTML = "--";
		document.getElementById("gprs_network_creg").innerHTML = "--";
		document.getElementById("gprs_network_csq").innerHTML = "--";
		document.getElementById("gprs_network_area").innerHTML = "--";
		document.getElementById("gprs_net_info_title").innerHTML = GPRS_OR_NBIOT + "网络(关机)";
	}
}

function Show_All_NetInfo()
{
	MyGetJSONWithArg("","/cgi-bin/getAllNetInfo?","", function (info)
	{
		if( info.tcpipinfo != null ) {
			Page_Root_GPRS(info.tcpipinfo);
			Page_Root_Network(info.tcpipinfo);
		}
		if( info.gprsinfo != null ) {
			Page_gprs_network(info.gprsinfo);
		}
		if( info.netinfo != null ) {
			Page_Network_Information(info.netinfo);
		}
	});
}

//GPRS 与 以太网 连接状态
function Show_Root_GPRS_Network()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Root_GPRS("12345");
	MyGetJSONWithArg("","/cgi-bin/getTcpipState?","", function (info)
	{
		Page_Root_GPRS(info.states);
		Page_Root_Network(info.states);
	});
}

function Page_Root_GPRS(obj)
{
	var str = "<table style='width:96%' class='list_info_table'>";
	str += "<tr><th >序号</th><th >远程IP</th><th>远程端口</th><th>本地IP</th><th>本地端口</th><th>状态</th><th>已连接:（时长）</th></tr>";

	try
	{
		for (var n = ENET_TCPIP_NUM; n < ENET_TCPIP_NUM + GPRS_TCPIP_NUM; n++)
		{
			str += "<tr>";
			str += "<td>" + (n-ENET_TCPIP_NUM+1) + "</td>";
			if( 0 == obj[n].en ) {
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>未启用</td>";
				str += "<td>--</td>";
			} else {
				str += "<td>" + obj[n].rip + "</td>";
				str += "<td>" + obj[n].rpt + "</td>";
				str += "<td>" + obj[n].lip + "</td>";
				str += "<td>" + obj[n].lpt + "</td>";
				switch(obj[n].st) {
				case 0:
					str += "<td>等待</td>";
					break;
				case 1:
					str += "<td>已连接</td>";
					break;
				case 2:
					str += "<td>正在连接</td>";
					break;
				case 3:
					str += "<td>等待连接</td>";
					break;
				}
				if( 1 == obj[n].st ) {
					str += "<td>" + (obj[n].tn-obj[n].tc) + " 秒</td>";
				} else {
					str += "<td>--</td>";
				}
			}
			str += "</tr>";
		}
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_gprs").innerHTML = str;

}


//GPRS连接状态
//function Show_Root_Network()
//{
//	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
//	Page_Root_Network("12345");
//}

function Page_Root_Network(obj)
{
	var str = "<table style='width:96%' class='list_info_table'>";
	str += "<tr><th style=''>序号</th><th style=''>远程IP</th><th style=''>远程端口</th><th style=''>本地IP</th><th style=''>本地端口</th><th >状态</th><th>已连接:（时长）</td></tr>";

	try
	{
		for (var n = 0; n < ENET_TCPIP_NUM; n++)
		{
			str += "<tr>";
			str += "<td>" + (n+1) + "</td>";
			if( 0 == obj[n].en ) {
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>--</td>";
				str += "<td>未启用</td>";
				str += "<td>--</td>";
			} else {
				str += "<td>" + obj[n].rip + "</td>";
				str += "<td>" + obj[n].rpt + "</td>";
				str += "<td>" + obj[n].lip + "</td>";
				str += "<td>" + obj[n].lpt + "</td>";
				switch(obj[n].st) {
				case 0:
					str += "<td>等待</td>";
					break;
				case 1:
					str += "<td>已连接</td>";
					break;
				case 2:
					str += "<td>正在连接</td>";
					break;
				case 3:
					str += "<td>等待连接</td>";
					break;
				}
				if( 1 == obj[n].st ) {
					str += "<td>" + (obj[n].tn-obj[n].tc) + " 秒</td>";
				} else {
					str += "<td>--</td>";
				}
			}
			str += "</tr>";
		}
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_network").innerHTML = str;

}

//时间同步
function Show_Time_Synchronization()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；	
	MyGetJSONWithArg("正在获取设备基本配置","/cgi-bin/getHostCfg?","",function( cfg ){
		Page_Time_Synchronization(cfg)
	});
}

function Page_Time_Synchronization(cfg)
{
	//主机名
	setValue('base_hostname', cfg.host);
	//设备ID
	setValue('base_devid', cfg.id);
	//设备时区
	setValue('base_timezone', cfg.tz);
	//时间同步
	setCheckBoxEnable("base_time", Number(cfg.sync)!=0);
	//候选NTP服务器1
	setValue('base_ntp1', cfg.ntp1);
	//候选NTP服务器2
	setValue('base_ntp2', cfg.ntp2);
	
	//setCheckBoxEnable("base_debug", (cfg.dbg != null) && (Number(cfg.dbg)!=0));
}

function Apply_Time_Synchronization()
{
	//主机名
	var base_hostname = document.getElementById("base_hostname").value;
	//设备ID
	var base_devid = document.getElementById("base_devid").value;
	//设备ID
	var base_timezone = document.getElementById("base_timezone").value;
	//时间同步
	var base_time = getChecked("base_time");
	//候选NTP服务器1
	var base_ntp1 = document.getElementById("base_ntp1").value;
	//候选NTP服务器2
	var base_ntp2 = document.getElementById("base_ntp2").value;
	
	//var base_debug = getChecked("base_debug");

	if (chk(base_hostname, "主机名")) return;
	//if (chk(base_devid, "设备ID")) return;
	if (chk(base_timezone, "时区设置")) return;
	if (chk(base_ntp1, "候选NTP服务器1")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		host:base_hostname, 
		id:base_devid, 
		tz:Number(base_timezone), 
		sync:base_time?1:0, 
		ntp1:base_ntp1, 
		ntp2:base_ntp2
	};
	
	MyGetJSONWithArg("正在设置设备基本配置","/cgi-bin/setHostCfg?", JSON.stringify(setval));
}


//历史数据保存
function Show_Storage_Cfg()
{
	MyGetJSONWithArg("正在获取历史数据保存配置","/cgi-bin/getStorageCfg?","",function( cfg ){
		Page_Storage_Cfg(cfg)
	});
}

function Page_Storage_Cfg(cfg)
{
	//是否存储
	setCheckBoxEnable("storage_en",Number(cfg.se)!=0);
	//存储间隔
	setValue('storage_step', cfg.ss);
	//存储位置
	setValue('storage_path', cfg.path);
}

//历史数据保存配置
function Apply_Storage_Cfg()
{
	//是否存储
	var storage_en = getChecked("storage_en");
	//存储间隔
	var storage_step = getValue("storage_step");
	//存储位置
	var storage_path = getValue("storage_path");

	if (chk(storage_step, "保存间隔")) return;
	if (chk(storage_path, "保存路径")) return;

		//注：在此处填写保存代码即可；
	var setval = {
		se:storage_en?1:0, 
		ss:Number(storage_step), 
		path:Number(storage_path)
	};
	
	MyGetJSONWithArg("正在设置历史数据保存配置","/cgi-bin/setStorageCfg?",JSON.stringify(setval),function( rsp ){
		if( rsp != null && rsp.ret != null && rsp.ret != 0 ) {
			alert( "配置失败,请检查SD卡是否插入正常" );
		}
	});
}

//日志
function Show_Log()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Log("注：此处传入一个对象");
}

function Page_Log(obj)
{

	//系统日志缓冲区大小 
	//setValue('log_size', obj.null);
	//chenqq
	setValue('log_size', null);
	//远程log服务器 
	//setValue('log_remote', obj.null);
	setValue('log_remote', null);
	//远程log服务器端口
	//setValue('log_port', obj.null);
	setValue('log_port', null);

	//日志记录等级
	var tmp_log_level = document.getElementById("log_level");
	for (var i = 0; i < tmp_log_level.options.length; i++)
	{
		if (tmp_log_level.options[i].value == "此处填写对象.值")
		{
			tmp_log_level.selectedIndex = i;
			break;
		}
	}

	//Cron日志级别 
	var log_cron = document.getElementById("log_cron");
	for (var i = 0; i < log_cron.options.length; i++)
	{
		if (log_cron.options[i].value == "此处填写对象.值")
		{
			log_cron.selectedIndex = i;
			break;
		}
	}
}

function Apply_Log()
{
	//系统日志缓冲区大小
	var log_size = document.getElementById("log_size").value;
	//远程log服务器 
	var log_remote = document.getElementById("log_remote").value;
	//远程log服务器端口
	var log_port = document.getElementById("log_port").value;
	//日志记录等级
	var v = document.getElementById("log_level").selectedIndex;
	var log_level_ = document.getElementById("log_level").options[v].text;

	//Cron日志级别
	v = document.getElementById("log_cron").selectedIndex;
	var log_cron_ = document.getElementById("log_cron").options[v].text;


	if (chk(log_size, "系统日志缓冲区大小")) return;
	if (chk(log_remote, "远程log服务器")) return;
	if (chk(log_port, "远程log服务器端口")) return;
	if (chk(log_level_, "日志记录等级")) return;
	if (chk(log_cron_, "Cron日志级别")) return;

	//注：在此处填写保存代码即可；
	msg();
}


//管理权限
function Show_Jurisdiction()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("正在获取管理者信息","/cgi-bin/getAuthCfg?","",function( info ){
		Page_Jurisdiction(info)
	});
}

function Page_Jurisdiction(info)
{
	//用户名 
	setValue('manage_user', info.user);
	//密码
	//setValue('manage_pwd', info.psk);
	//SSH访问
	document.getElementsByName("manage_ssh")[0].checked = (Number(info.ssh)!=0);//此处填写值；
	document.getElementsByName("manage_ssh")[1].checked = !(Number(info.ssh)!=0);//此处填写值；
	//端口
	setValue('manage_port', info.sp);
	//允许SSH密码验证
	document.getElementsByName("manage_ssh_cer")[0].checked = (Number(info.sshcer)!=0);;//此处填写值；
	document.getElementsByName("manage_ssh_cer")[1].checked = !(Number(info.sshcer)!=0);//此处填写值；
	//时间设置 
	setValue('manage_time', info.t);
}


function Apply_Jurisdiction()
{
	//用户名
	var manage_user = document.getElementById("manage_user").value;
	//密码 
	var manage_pwd = document.getElementById("manage_pwd").value;
	var manage_pwd_1 = document.getElementById("manage_pwd_1").value;
	//SSH访问
	var manage_ssh1 = document.getElementsByName("manage_ssh")[0].checked;
	var manage_ssh2 = document.getElementsByName("manage_ssh")[1].checked;
	//端口
	var manage_port = document.getElementById("manage_port").value;
	//允许SSH密码验证
	var manage_ssh_cer1 = document.getElementsByName("manage_ssh_cer")[0].checked;
	var manage_ssh_cer2 = document.getElementsByName("manage_ssh_cer")[1].checked;
	//时间设置
	var manage_time = document.getElementById("manage_time").value;


	if (chk(manage_user, "用户名")) return;
	if (chk(manage_pwd, "密码")) return;
	if (chk(manage_pwd_1, "确认密码")) return;
	if (manage_pwd != manage_pwd_1) {
		alert("密码与确认密码不一致！");
		return;
	}
	if (chk2(manage_ssh1, manage_ssh2, "SSH访问")) return;
	if (chk(manage_port, "端口")) return;
	if (chk2(manage_ssh_cer1, manage_ssh_cer2, "允许SSH密码验证")) return;
	if (chk(manage_time, "时间设置")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		user:manage_user, 
		psk:manage_pwd, 
		ssh:manage_ssh1?1:0, 
		sp:Number(manage_port), 
		sshcer:manage_ssh_cer1?1:0,
		t:Number(manage_time)
	};
	
	MyGetJSONWithArg("正在设置管理者信息","/cgi-bin/setAuthCfg?",JSON.stringify(setval),function( info ){
		if( info.ret != null && info.ret == 0 ) {
			alert("修改成功,需要重新登录!");
			window.location.href="http://"+window.location.host+"/login.html";
		}
	});
}


function Show_Local_Setup()
{
	var v = Number(document.getElementById("local_dhcp").value);
	document.getElementById("local_ip").disabled = (v>0);
	document.getElementById("local_mask").disabled = (v>0);
	document.getElementById("local_gateway").disabled = (v>0);
	document.getElementById("local_dns").disabled = (v>0);
}

//远程传输设置
function Show_Remote_Transmission()
{
	//每次选择时，都会调用该函数；
	//获取【远程传输参数集】；=====================================================================>需要调用【设备接口】获取
	//如下是例子，例如：
	//Tabel_Remote_Parameter(new Array(new Array("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH", "JJJ"), new Array("KKK", "LLL", "MMM", "OOO", "PPP", "QQQ", "RRR", "SSS", "TTT"), new Array("UUU", "VVV", "WWW")));
	//-----------------------------------------------------------------------------------------------------------------------

	var v = document.getElementById("remote_group").selectedIndex;
	var setval = { n:Number(v) };
	MyGetJSONWithArg("正在获取以太网 TCP/IP配置","/cgi-bin/getTcpipCfg?",JSON.stringify(setval),function( info ){
		Page_Remote_Transmission(info)
	});
}

function Enable_Remote_Transmission()
{
	var v = !getChecked("remote_group_is");

	document.getElementById("remote_socket_type").disabled = v;
	document.getElementById("remote_working_type").disabled = v;
	document.getElementById("remote_working_cs").disabled = v;
	document.getElementById("remote_proto").disabled = v;
	document.getElementById("remote_proto_ms").disabled = v;

	document.getElementById("remote_ip").disabled = v;
	document.getElementById("remote_port").disabled = v;
	document.getElementById("remote_upload_interval").disabled = v;
	document.getElementById("remote_heartbeat").disabled = v;
	
	show_remote_btn();
}

var Remote_Parameter_Items = 0;
function Tabel_Remote_Parameter(obj)
{
	var str = "<table id='table_remote_upload' style='display:none'>";

	try
	{
		var k = 0;
		for (var n = 0; n < obj.length; n++)
		{
			str += "<tr>";
			for (var j = 0; j < obj[n].length; j++)
			{
				str += "<td><input name='remote_upload_data' type='checkbox' id='remote_upload_data" + k + "' value='" + k + "' />&nbsp;参数" + obj[n][j] + "</td>";
				k++;
			}
			str += "</tr>";
		}
		Remote_Parameter_Items = k;
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_remote_parameter").innerHTML = str;

}

function Page_Remote_Transmission(cfg)
{
	//是否启用 
	setCheckBoxEnable("remote_group_is", Number(cfg.en)!=0);
	//以太网工作模式
	setValue('remote_socket_type', cfg.tt);
	setValue('remote_working_cs', cfg.cs);
	//设备NM号（0-20位）
	//setValue('remote_dev', obj.null);
	//域名地址
	setValue('remote_ip', cfg.pe);
	//域名端口
	setValue('remote_port', cfg.po);
	//上传间隔
	setValue('remote_upload_interval', cfg.it);
	//是否启用心跳包 
	setCheckBoxEnable("remote_heartbeat", Number(cfg.kl)!=0);
	
	setValue('remote_working_type', cfg.md);
	//协议
	if(cfg.md == TCP_IP_M_NORMAL) {
		if( cfg.normal.pt != null ) setValue('remote_proto', cfg.normal.pt);
		if( cfg.normal.ms != null ) setValue('remote_proto_ms', cfg.normal.ms);
		if( cfg.normal.mad != null ) setValue('remote_modbus_addr', cfg.normal.mad);
	} else if(cfg.md == TCP_IP_M_XFER) {
		setValue('remote_xfer_mode', cfg.xfer.md);
		if( cfg.xfer.pt != null ) setValue('remote_wm_xfer_proto_type', cfg.xfer.pt);
		if( cfg.xfer.dt != null ) setValue('remote_xfer_gw_trt_dst_type', cfg.xfer.dt);
		if( cfg.xfer.tidx != null ) setValue('remote_xfer_gw_trt_dst_gprs_index', cfg.xfer.tidx);
		if( cfg.xfer.uty != null ) setValue('remote_xfer_gw_trt_dst_uart_type', cfg.xfer.uty);
		if( cfg.xfer.ubr != null ) setValue('remote_xfer_gw_trt_dst_uart_baudrate', cfg.xfer.ubr);
		if( cfg.xfer.udb != null && cfg.xfer.usb != null  && cfg.xfer.upy != null ) setValue('remote_xfer_gw_trt_dst_uart_bits', (Number(cfg.xfer.udb) * 100) + (Number(cfg.xfer.upy) * 10) + Number(cfg.xfer.usb));
	}
	
	MyGetJSONWithArg("","/cgi-bin/getXferUartCfg?","", function (info)
	{
		Show_xfer_uart_cfg('', info.cfg);
	});

	Enable_Remote_Transmission();
}

var table_remote_upload_turnoff = 0;
function Show_Remote_Transmission_downlist()
{
	if (table_remote_upload_turnoff == 0)
	{
		table_remote_upload_turnoff = 1;
		document.getElementById("table_remote_upload").style.display = "";
	}
	else
	{
		table_remote_upload_turnoff = 0;
		document.getElementById("table_remote_upload").style.display = "none";
	}
	
}

function nUartGetIndex(instance)
{
	switch (instance) {
	case 0:
		return PROTO_DEV_RS1;
		break;
	case 1:
		return PROTO_DEV_RS2;
		break;
	case 2:
		return PROTO_DEV_RS3;
		break;
	case 3:
		return PROTO_DEV_RS4;
		break;
	}

	return -1;
}

function nUartGetInstance(index)
{
	switch (index) {
	case PROTO_DEV_RS1:
		return 0;
		break;
	case PROTO_DEV_RS2:
		return 1;
		break;
	case PROTO_DEV_RS3:
		return 2;
		break;
	case PROTO_DEV_RS4:
		return 3;
		break;
	}
	return -1;
}

function Apply_xfer_uart_cfg(_prefix)
{
	var uart_set = {
		cfg:new Array()
	};
	for(var i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++) {
		uart_set.cfg[i] = {'n':i,'en':0,'cnt':0,'addrs':''};
	}
	
	for(var i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++) {
		var _addrs_str = getValue(_prefix+'xfer_uart_cfgs_'+i);
		uart_set.cfg[i] = {
		   'n':i,
		   'en':(_addrs_str.length>0)?1:0, 
		   'cnt':_addrs_str.length>0?_addrs_str.split(",").length:0,
		   'addrs':_addrs_str
		};
	}
	
	MyGetJSONWithArg("正在设置串口地址表","/cgi-bin/setXferUartCfg?",JSON.stringify(uart_set));
}

function Show_xfer_uart_cfg(_prefix, _cfg)
{
	for(var i = 0; i < _cfg.length; i++) {
		var n = _cfg[i]['n'];
		if(n <= PROTO_DEV_RS_MAX) {
			setValue(_prefix+'xfer_uart_cfgs_'+n, _cfg[i].addrs!=null?_cfg[i].addrs:"");
		} else if(PROTO_DEV_ZIGBEE == n) {
			setValue(_prefix+'xfer_uart_cfgs_zgb', _cfg[i].addrs!=null?_cfg[i].addrs:"");
		}
	}
}


function Apply_Remote_Transmission_downlist()
{
	//是否启用
	var remote_group_is = getChecked("remote_group_is");
	//以太网工作模式 
	var remote_socket_type = getValue("remote_socket_type");
	var remote_working_type = getValue("remote_working_type");
	var remote_working_cs = getValue("remote_working_cs");

	//设备NM号（0-20位）
	//var remote_dev = getValue("remote_dev");
	//域名地址
	var remote_ip = getValue("remote_ip");
	//域名端口
	var remote_port = getValue("remote_port");
	//上传间隔
	var remote_upload_interval = getValue("remote_upload_interval");
	
	//是否启用心跳包
	var remote_heartbeat = getChecked("remote_heartbeat");
	
	//协议
	var remote_proto = getValue("remote_proto");
	var remote_proto_ms = getValue("remote_proto_ms");
	var remote_modbus_addr = getValue("remote_modbus_addr");
	
	var remote_wm_xfer_proto_type = getValue("remote_wm_xfer_proto_type");
	var remote_xfer_gw_trt_dst_type = getValue("remote_xfer_gw_trt_dst_type");
	var remote_xfer_gw_trt_dst_uart_type = getValue("remote_xfer_gw_trt_dst_uart_type");
	var remote_xfer_gw_trt_dst_uart_baudrate = getValue("remote_xfer_gw_trt_dst_uart_baudrate");
	var remote_xfer_gw_trt_dst_uart_bits = getValue("remote_xfer_gw_trt_dst_uart_bits");
	var remote_xfer_gw_trt_dst_gprs_index = getValue("remote_xfer_gw_trt_dst_gprs_index");

	var md = Number(remote_working_type);
	
	if( remote_group_is ) {
		if (chk2(remote_socket_type, remote_working_cs, "以太网套接字类型")) return;
		if (chk(remote_working_type, "以太网通讯模式")) return;
		if( Number(remote_working_cs) != 1 ) {
			if (chk(remote_ip, "域名地址")) return;
		}
		if (chk(remote_port, "域名端口")) return;
		if (chk(remote_upload_interval, "上传间隔")) return;
		//if (chk(remote_parameter, "上传的数据")) return;

		if(md == TCP_IP_M_NORMAL) {
			if(Number(remote_proto) == 1 ) {
				remote_proto_ms = 0;
			}
			if (chk2(remote_proto, remote_proto_ms, "协议")) return;
			if( Number(remote_proto)==PROTO_MODBUS_RTU_OVER_TCP&&Number(remote_proto_ms)==0) {
				if (chk(remote_modbus_addr, "Modbus 从机地址")) return;
			}
		} else if(md == TCP_IP_M_XFER) {
			var is_gw = getNumber('remote_xfer_mode')== XFER_M_GW;
			var is_trt = getNumber('remote_xfer_mode')== XFER_M_TRT;
			var dst_type = -1;
			if(is_gw||is_trt) {
				dst_type = getNumber('remote_xfer_gw_trt_dst_type');
			}
			if((is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX) {
				//if(chk(remote_xfer_gw_trt_dst_uart_type, "转发端口")) return;
				if(chk(remote_xfer_gw_trt_dst_uart_baudrate, "转发波特率")) return;
				if(chk(remote_xfer_gw_trt_dst_uart_bits, "转发数据位")) return;
			}
			if((is_gw||is_trt)&&dst_type==PROTO_DEV_GPRS) {
				if(chk(remote_xfer_gw_trt_dst_gprs_index, "转发目标GPRS/LTE组号")) return;
			}
			if(is_gw||is_trt) {
				if(chk(remote_xfer_gw_trt_dst_type, "转发端口")) return;
			}
			if(is_gw) {
				if(chk(remote_wm_xfer_proto_type, "转发协议")) return;
			}
		}
	}

	//注：在此处填写保存代码即可；
	var n = document.getElementById("remote_group").selectedIndex;
	
	var setval = {
		n:Number(n), 
		en:remote_group_is?1:0, 
		md:Number(remote_working_type), 
		tt:Number(remote_socket_type), 
		cs:Number(remote_working_cs), 
		pe:remote_ip, 
		po:Number(remote_port), 
		it:Number(remote_upload_interval), 
		kl:remote_heartbeat?1:0
	};
	
	if(md == TCP_IP_M_NORMAL) {
		setval['normal'] = {
			pt:Number(remote_proto), 
			ms:Number(remote_proto_ms), 
			mad:Number(remote_modbus_addr)
		}
	} else if(md == TCP_IP_M_XFER) {
		setval['xfer'] = {
			md:getNumber('remote_xfer_mode'), 
			pt:Number(remote_wm_xfer_proto_type), 
			dt:Number(remote_xfer_gw_trt_dst_type), 
			tidx:Number(remote_xfer_gw_trt_dst_gprs_index), 
			ubr:Number(remote_xfer_gw_trt_dst_uart_baudrate), 
			udb:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(0)), 
			upy:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(1)), 
			usb:parseInt(remote_xfer_gw_trt_dst_uart_bits.charAt(2))
		}
	}
	
	MyGetJSONWithArg("正在设置以太网 TCP/IP配置","/cgi-bin/setTcpipCfg?",JSON.stringify(setval));
}


//本地网络配置
function Show_Local_Network_Configuration()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Local_Network_Configuration("注：此处传入一个对象");
	MyGetJSONWithArg("","/cgi-bin/getNetCfg?","", function (info)
	{
		Page_Local_Network_Configuration(info)
	});
}

function Page_Local_Network_Configuration(cfg)
{
	//DHCP开关
	setValue('local_dhcp', cfg.dh);
	//IP 地址
	setValue('local_ip', cfg.ip);
	//子网掩码
	setValue('local_mask', cfg.mask);
	//网关
	setValue('local_gateway', cfg.gw);
	//DNS
	setValue('local_dns', cfg.dns);
	
	Show_Local_Setup();
}

function Apply_Local_Network_Configuration()
{
	//IP 地址
	var local_dhcp = document.getElementById("local_dhcp").value;
	//IP 地址
	var local_ip = document.getElementById("local_ip").value;
	//子网掩码
	var local_mask = document.getElementById("local_mask").value;
	//网关
	var local_gateway = document.getElementById("local_gateway").value;
	//DNS
	var local_dns = document.getElementById("local_dns").value;

	if (chk(local_dhcp, "DHCP开关")) return;
	if( Number(local_dhcp) != 1 ) {
		if (chk(local_ip, "IP 地址")) return;
		if (chk(local_mask, "子网掩码")) return;
		if (chk(local_gateway, "网关")) return;
		if (chk(local_dns, "DNS")) return;
	}
	
	var setval = {
		dh:Number(local_dhcp), 
		ip:local_ip, 
		mask:local_mask, 
		gw:local_gateway, 
		dns:local_dns
	};
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置网络,请稍后...","/cgi-bin/setNetCfg?",JSON.stringify(setval));
}


//ZIGBEE 配置
function Show_ZIGBEE()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；

	MyGetJSONWithArg("正在获取Zigbee配置","/cgi-bin/getZigbeeCfg?", "", function( cfg ){
		if(cfg != null && cfg.ret != 0) {
			alert("Zigbee 初始化失败,请检查后再重试。");
		} else {
			MyGetJSONWithArg("","/cgi-bin/getXferUartCfg?","", function (info)
			{
				Show_xfer_uart_cfg('zgb_', info.cfg);
			});
			Page_ZIGBEE(cfg);
		}
	});
}

function zgb_tmode_change() {
	var ZIGBEE_Working_Mode = getNumber("ZIGBEE_Working_Mode");
	var is_trt = getNumber('ZIGBEE_T_Mode')==ZGB_TM_TRT;
	var is_dtu = getNumber('ZIGBEE_T_Mode')==ZGB_TM_DTU;
	setDisplay('zgb_xfer_uart_cfgs_0_ul', is_dtu&&(ZIGBEE_Working_Mode!=2));
	setDisplay('zgb_xfer_uart_cfgs_1_ul', is_dtu&&(ZIGBEE_Working_Mode!=2));
	setDisplay('zgb_xfer_uart_cfgs_2_ul', is_dtu&&(ZIGBEE_Working_Mode!=2));
	setDisplay('zgb_xfer_uart_cfgs_3_ul', is_dtu&&(ZIGBEE_Working_Mode!=2));
	setDisplay('zgb_xfer_trt_ul', is_trt&&(ZIGBEE_Working_Mode!=2));
	setDisplay('zgb_slave_address_ul', !is_trt&&(ZIGBEE_Working_Mode!=2));
}

function ZIGBEE_Working_Mode_change() {
	var ZIGBEE_Working_Mode = getNumber("ZIGBEE_Working_Mode");
	setDisplay('ZIGBEE_LearnStep_ul', ZIGBEE_Working_Mode == 2 );
	setDisplay('ZIGBEE_Destination_Address_ul', ZIGBEE_Working_Mode != 2 );
	setDisplay('ZIGBEE_Slave_Address_ul', ZIGBEE_Working_Mode != 2 );
	if(ZIGBEE_Working_Mode == 2) setValue('ZIGBEE_T_Mode', '0');
	zgb_tmode_change();
}

function Page_ZIGBEE(cfg)
{
	setDisplay('ZIGBEE_LearnStep_ul', cfg.wmd == 2 );
	//ZIGBEE工作模式
	setValue('ZIGBEE_Working_Mode',cfg.wmd);
	setValue('ZIGBEE_T_Mode',cfg.tm);
	setValue('ZIGBEE_Slave_Address',cfg.slvad);
	// 学习间隔
	setValue('ZIGBEE_LearnStep',cfg.lstep);
	//panID
	setValue('ZIGBEE_panID', cfg.pid);
	//本地网络地址
	setValue('ZIGBEE_Local_Address', cfg.src);
	//本地MAC地址
	setValue('ZIGBEE_Local_Mac', cfg.smac);
	//目的网络地址
	setValue('ZIGBEE_Destination_Address', cfg.dst);
	//目的MAC地址
	setValue('ZIGBEE_Destination_Mac', cfg.dmac);
	//信道
	setValue('ZIGBEE_Channel', Number(cfg.ch)-11);
	//发送模式
	setValue('ZIGBEE_Send_Mode', cfg.mmd);
	
	setDisplay('ZIGBEE_LearnStep_ul', _tonum_(cfg.wmd) == 2 );
	setDisplay('ZIGBEE_Destination_Address_ul', _tonum_(cfg.wmd) != 2 );
	setDisplay('ZIGBEE_Slave_Address_ul', _tonum_(cfg.wmd) != 2 );
	
	zgb_tmode_change();
	
	var is_trt = getNumber('ZIGBEE_T_Mode')==ZGB_TM_TRT;
	if(is_trt) {
		setValue('zgb_xfer_trt_dst_type', cfg.dt);
		setValue('zgb_xfer_trt_dst_uart_baudrate', cfg.ubr);
		setValue('zgb_xfer_trt_dst_uart_bits', (Number(cfg.udb) * 100) + (Number(cfg.upy) * 10) + Number(cfg.usb));
	}
}

function Apply_ZIGBEE()
{
	//ZIGBEE工作模式
	var ZIGBEE_Working_Mode = getValue("ZIGBEE_Working_Mode");
	if(Number(ZIGBEE_Working_Mode) == 2) {
		setValue('ZIGBEE_T_Mode', '0');
		setValue('ZIGBEE_Destination_Address', '65535');
	}
	var ZIGBEE_T_Mode = getValue("ZIGBEE_T_Mode");
	var ZIGBEE_Slave_Address = getValue("ZIGBEE_Slave_Address");
	
	//学习间隔
	var ZIGBEE_LearnStep = getValue("ZIGBEE_LearnStep");

	//panID
	var ZIGBEE_panID = getValue("ZIGBEE_panID");
	//本地网络地址
	var ZIGBEE_Local_Address = getValue("ZIGBEE_Local_Address");
	//本地MAC地址
	var ZIGBEE_Local_Mac = getValue("ZIGBEE_Local_Mac");
	//目的网络地址
	var ZIGBEE_Destination_Address = getValue("ZIGBEE_Destination_Address");
	//目的MAC地址
	var ZIGBEE_Destination_Mac = getValue("ZIGBEE_Destination_Mac");
	//信道
	var ZIGBEE_Channel = getValue("ZIGBEE_Channel");
	//发送模式
	var ZIGBEE_Send_Mode = getValue("ZIGBEE_Send_Mode");
	
	var is_trt = getNumber('ZIGBEE_T_Mode')==ZGB_TM_TRT;
	
	var zgb_xfer_trt_dst_type = getValue("zgb_xfer_trt_dst_type");
	var zgb_xfer_trt_dst_uart_baudrate = getValue("zgb_xfer_trt_dst_uart_baudrate");
	var zgb_xfer_trt_dst_uart_bits = getValue("zgb_xfer_trt_dst_uart_bits");
	
	if (chk(ZIGBEE_Working_Mode, "ZIGBEE工作模式")) return;
	if ( ZIGBEE_Working_Mode == 2 && chk(ZIGBEE_LearnStep, "ZIGBEE协调器自学习间隔")) return;
	if ( ZIGBEE_Working_Mode != 2 && chk(ZIGBEE_Slave_Address, "ZIGBEE 从机地址") && chk(ZIGBEE_T_Mode, "通讯模式")) return;
	if (chk(ZIGBEE_panID, "panID")) return;
	if (chk(ZIGBEE_Local_Address, "本地网络地址")) return;
	if (chk(ZIGBEE_Destination_Address, "目的网络地址")) return;
	if (chk(ZIGBEE_Destination_Mac, "目的MAC地址")) return;
	if (chk(ZIGBEE_Channel, "信道")) return;
	if (chk(ZIGBEE_Send_Mode, "发送模式")) return;
	
	if(is_trt) {
		if(chk(zgb_xfer_trt_dst_type, "目标端口")) return;
		if(chk(zgb_xfer_trt_dst_uart_baudrate, "目标端口波特率")) return;
		if(chk(zgb_xfer_trt_dst_uart_bits, "目标端口数据位")) return;
	} else {
		zgb_xfer_trt_dst_type = '0';
		zgb_xfer_trt_dst_uart_baudrate = '115200';
		zgb_xfer_trt_dst_uart_bits = '800';
	}

	//注：在此处填写保存代码即可；
	
	var setval = {
		wmd:Number(ZIGBEE_Working_Mode), 
		slvad:Number(ZIGBEE_Slave_Address), 
		lstep:Number(ZIGBEE_LearnStep), 
		pid:Number(ZIGBEE_panID), 
		src:Number(ZIGBEE_Local_Address), 
		smac:ZIGBEE_Local_Mac, 
		dst:Number(ZIGBEE_Destination_Address), 
		dmac:ZIGBEE_Destination_Mac, 
		ch:(Number(ZIGBEE_Channel)+11), 
		mmd:Number(ZIGBEE_Send_Mode), 
		tm:Number(ZIGBEE_T_Mode),
		dt:Number(zgb_xfer_trt_dst_type), 
		ubr:Number(zgb_xfer_trt_dst_uart_baudrate), 
		udb:parseInt(zgb_xfer_trt_dst_uart_bits.charAt(0)), 
		upy:parseInt(zgb_xfer_trt_dst_uart_bits.charAt(1)), 
		usb:parseInt(zgb_xfer_trt_dst_uart_bits.charAt(2))
	};
	
	if(ZIGBEE_Working_Mode != 2 && setval.tm == ZGB_TM_DTU) {
		Apply_xfer_uart_cfg('zgb_');
	}
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置Zigbee,请稍后...","/cgi-bin/setZigbeeCfg?",JSON.stringify(setval));
}




//LORA配置
function Show_LORA()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	Page_LORA("注：此处传入一个对象");
}

function Page_LORA(obj)
{
	//载波频率
	//change by chenqq
	//setValue('LORA_Carrier_Frequency', obj.null);
	setValue('LORA_Carrier_Frequency', null);
	//扩频因子
	//setValue('LORA_Spreading_Factor', obj.null);
	setValue('LORA_Spreading_Factor', null);
	//工作模式
	var LORA_Working_Mode = document.getElementById("LORA_Working_Mode");
	for (var i = 0; i < LORA_Working_Mode.options.length; i++)
	{
		if (LORA_Working_Mode.options[i].value == "此处填写对象.值")
		{
			LORA_Working_Mode.selectedIndex = i;
			break;
		}
	}
	//扩频带宽
	//setValue('LORA_Spread', obj.null);
	setValue('LORA_Spread', null);
	//用户ID
	//setValue('LORA_USERID', obj.null);
	setValue('LORA_USERID', null);
	//网络ID
	//setValue('LORA_NETWORKID', obj.null);
	setValue('LORA_NETWORKID', null);
	//发射功率
	var LORA_Transmit_Power = document.getElementById("LORA_Transmit_Power");
	for (var i = 0; i < LORA_Transmit_Power.options.length; i++)
	{
		if (LORA_Transmit_Power.options[i].value == "此处填写对象.值")
		{
			LORA_Transmit_Power.selectedIndex = i;
			break;
		}
	}

}


function Apply_LORA()
{
	//载波频率
	var LORA_Carrier_Frequency = document.getElementById("LORA_Carrier_Frequency").value;
	//扩频因子
	var LORA_Spreading_Factor = document.getElementById("LORA_Spreading_Factor").value;
	//工作模式
	var v = document.getElementById("LORA_Working_Mode").selectedIndex;
	var LORA_Working_Mode_ = document.getElementById("LORA_Working_Mode").options[v].text;
	
	//扩频带宽
	var LORA_Spread = document.getElementById("LORA_Spread").value;
	//用户ID
	var LORA_USERID = document.getElementById("LORA_USERID").value;
	//网络ID
	var LORA_NETWORKID = document.getElementById("LORA_NETWORKID").value;
	//发射功率
	v = document.getElementById("LORA_Transmit_Power").selectedIndex;
	var LORA_Transmit_Power_ = document.getElementById("LORA_Transmit_Power").options[v].text;
	
	if (chk(LORA_Carrier_Frequency, "载波频率")) return;
	if (chk(LORA_Spreading_Factor, "扩频因子")) return;
	if (chk(LORA_Working_Mode_, "工作模式")) return;
	if (chk(LORA_Spread, "扩频带宽")) return;
	if (chk(LORA_USERID, "用户ID")) return;
	if (chk(LORA_NETWORKID, "网络ID")) return;
	if (chk(LORA_Transmit_Power_, "发射功率")) return;


	//注：在此处填写保存代码即可；
	msg();
}

function Apply_Network_Adapter_Configuration()
{
	//adapter
	var adpt = getNumber('net_adapter');

	var setval = {
		adpt:adpt
	};
	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在配置网络适配器,请稍后...","/cgi-bin/setNetCfg?",JSON.stringify(setval));
}

//GPRS 配置——工作模式及中心配置
function Show_GPRS_Transmission()
{
	//每次选择时，都会调用该函数；
	//获取【GPRS 配置——工作模式及中心配置】；=====================================================================>需要调用【设备接口】获取
	//如下是例子，例如：
	//Tabel_GPRS_Parameter(new Array(new Array("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH", "JJJ"), new Array("KKK", "LLL", "MMM", "OOO", "PPP", "QQQ", "RRR", "SSS", "TTT"), new Array("UUU", "VVV", "WWW")));
	//-----------------------------------------------------------------------------------------------------------------------

	var val = document.getElementById("GPRS_group").value;
	var setval = { n:(Number(val)+ENET_TCPIP_NUM) };
	
	MyGetJSONWithArg("正在获取GRPS TCP/IP配置","/cgi-bin/getTcpipCfg?",JSON.stringify(setval), function (info)
	{
		Page_GPRS_Transmission(info)
	});
}


var GPRS_Parameter_Items = 0;
function Tabel_GPRS_Parameter(obj)
{
	var str = "<table id='table_gprs_upload' style='display:none'>";

	try
	{
		var k = 0;
		for (var n = 0; n < obj.length; n++)
		{
			str += "<tr>";
			for (var j = 0; j < obj[n].length; j++)
			{
				str += "<td><input name='GPRS_upload_data' type='checkbox' id='GPRS_upload_data" + k + "' value='" + k + "' />&nbsp;参数" + obj[n][j] + "</td>";
				k++;
			}
			str += "</tr>";
		}
		GPRS_Parameter_Items = k;
	}
	catch (e)
	{
	}

	str += "</table>";

	document.getElementById("div_gprs_parameter").innerHTML = str;

}

function GPRS_proto_change() {
	setDisplay('GPRS_proto_ms_addr', (getNumber('GPRS_proto_ms')== 0)&&getNumber('GPRS_proto')==PROTO_MODBUS_RTU_OVER_TCP);
}

function GPRS_working_type_change() {
	setDisplay('GPRS_wm_normal_ul', getNumber('GPRS_working_type') == 0 );
	setDisplay('GPRS_wm_xfer_ul', getNumber('GPRS_working_type') == 1 );
}

function GPRS_xfer_gw_trt_dst_type_change() {
	var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
	var dst_type = -1;
	if(is_gw||is_trt) {
		dst_type = getNumber('GPRS_xfer_gw_trt_dst_type');
	}
	setDisplay('GPRS_xfer_gw_trt_dst_net_index_ul', (is_gw||is_trt)&&dst_type==PROTO_DEV_NET);
	setDisplay('GPRS_xfer_gw_trt_dst_uart_type_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('GPRS_xfer_gw_trt_dst_uart_baudrate_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('GPRS_xfer_gw_trt_dst_uart_bits_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
}

function GPRS_xfer_mode_change() {
	var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
	setDisplay('GPRS_wm_xfer_proto_type_ul', is_gw);
	setDisplay('GPRS_xfer_gw_trt_dst_type_ul', is_gw||is_trt);
	GPRS_xfer_gw_trt_dst_type_change();
}

function show_GPRS_btn(){
	setDisplay('show_GPRS_dialog_btn', false);
	setDisplay('GPRS_proto_ms_ul', true);
	setEnable('GPRS_working_cs', true);
	setEnable('GPRS_socket_type', true);
	switch(getNumber('GPRS_proto')) {
	case PROTO_CC_BJDC: case PROTO_HJT212: case PROTO_DM101:
		setDisplay('show_GPRS_dialog_btn', true);
		setEnable('GPRS_working_cs', false);
		if(__is_gprs()) {
			setEnable('GPRS_socket_type', false);
		}
		setDisplay('GPRS_proto_ms_ul', false);
		setValue('GPRS_working_cs', 0);
		if(__is_gprs()) {
			setValue('GPRS_socket_type', 0);
		}
		break;
	}
	GPRS_proto_change();
	GPRS_working_type_change();
	GPRS_xfer_mode_change();
}

//通讯协议弹窗
function show_GPRS_dialog(){
		setGPRSFileToTextarea('');
		showDialog('GPRS_dialog');

		$("#dialog_GPRS_ok_btn").off().on("click", function(){
			saveGPRSConfig();
			hideDialog('GPRS_dialog');
		})
}

function setGPRSFileToTextarea()
{
	if(getValue('GPRS_proto')==PROTO_HJT212) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_hjt212_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	} else if (getValue('GPRS_proto')==PROTO_DM101) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dm101_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#GPRS_config_textarea").val(data);
			}
		}); 
	}
}

function saveGPRSConfig()
{
	var _val = $("#GPRS_config_textarea").val();
	var xhr = createXMLHttpRequest();
	if (xhr) {
		Show("正在保存配置,请稍后...");
		xhr.onreadystatechange = function() {
			if( xhr.readyState==4 ) {
				Close();
				if( xhr.status==200 ) {
					alert("保存成功，重启生效！");
				} else {
					alert("失败,请重试");
				}
			}
		}
		if(getValue('GPRS_proto')==PROTO_HJT212) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_hjt212_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		} else if (getValue('GPRS_proto')==PROTO_DM101) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dm101_"+(ENET_TCPIP_NUM+getNumber('GPRS_group'))+".ini" );
		}
		xhr.send(_val);
	}
}

function Enable_GPRS_Transmission()
{
	var v = !getChecked("GPRS_group_is");

	document.getElementById("GPRS_socket_type").disabled = v;
	document.getElementById("GPRS_working_cs").disabled = v;
	document.getElementById("GPRS_proto").disabled = v;
	document.getElementById("GPRS_proto_ms").disabled = v;

	document.getElementById("GPRS_peer_ip").disabled = v;
	document.getElementById("GPRS_peer_port").disabled = v;
	document.getElementById("GPRS_upload_interval").disabled = v;
	document.getElementById("GPRS_heartbeat").disabled = v;

	show_GPRS_btn();
}


function Page_GPRS_Transmission(cfg)
{
	//是否启用 
	setCheckBoxEnable("GPRS_group_is", Number(cfg.en)!=0);
	//以太网工作模式
	setValue('GPRS_socket_type', cfg.tt);
	setValue('GPRS_working_cs', cfg.cs);
	//设备NM号（0-20位）
	//setValue('remote_dev', obj.null);
	//域名地址
	setValue('GPRS_peer_ip', cfg.pe);
	//域名端口
	setValue('GPRS_peer_port', cfg.po);
	//上传间隔
	setValue('GPRS_upload_interval', cfg.it);

	//是否启用心跳包 
	setCheckBoxEnable("GPRS_heartbeat", Number(cfg.kl)!=0);

	setValue('GPRS_working_type', cfg.md);
	//协议
	if(cfg.md == TCP_IP_M_NORMAL) {
		if( cfg.normal.pt != null ) setValue('GPRS_proto', cfg.normal.pt);
		if( cfg.normal.ms != null ) setValue('GPRS_proto_ms', cfg.normal.ms);
		if( cfg.normal.mad != null ) setValue('GPRS_modbus_addr', cfg.normal.mad);
	} else if(cfg.md == TCP_IP_M_XFER) {
		setValue('GPRS_xfer_mode', cfg.xfer.md);
		if( cfg.xfer.pt != null ) setValue('GPRS_wm_xfer_proto_type', cfg.xfer.pt);
		if( cfg.xfer.dt != null ) setValue('GPRS_xfer_gw_trt_dst_type', cfg.xfer.dt);
		if( cfg.xfer.tidx != null ) setValue('GPRS_xfer_gw_trt_dst_net_index', cfg.xfer.tidx);
		if( cfg.xfer.uty != null ) setValue('GPRS_xfer_gw_trt_dst_uart_type', cfg.xfer.uty);
		if( cfg.xfer.ubr != null ) setValue('GPRS_xfer_gw_trt_dst_uart_baudrate', cfg.xfer.ubr);
		if( cfg.xfer.udb != null && cfg.xfer.usb != null  && cfg.xfer.upy != null ) setValue('GPRS_xfer_gw_trt_dst_uart_bits', (Number(cfg.xfer.udb) * 100) + (Number(cfg.xfer.upy) * 10) + Number(cfg.xfer.usb));
	}
	
	MyGetJSONWithArg("","/cgi-bin/getXferUartCfg?","", function (info)
	{
		Show_xfer_uart_cfg('GPRS_', info.cfg);
	});
	
	Enable_GPRS_Transmission();
}


var table_gprs_upload_turnoff = 0;
function Show_gprs_Transmission_downlist()
{
	if (table_gprs_upload_turnoff == 0)
	{
		table_gprs_upload_turnoff = 1;
		document.getElementById("table_gprs_upload").style.display = "";
	}
	else
	{
		table_gprs_upload_turnoff = 0;
		document.getElementById("table_gprs_upload").style.display = "none";
	}

}


function Apply_gprs_Transmission_downlist()
{
	//是否启用
	var GPRS_group_is = getChecked("GPRS_group_is");
	//以太网工作模式 
	var GPRS_working_type = getValue("GPRS_working_type");
	var GPRS_socket_type = getValue("GPRS_socket_type");
	var GPRS_working_cs = getValue("GPRS_working_cs");

	//设备NM号（0-20位）
	//var GPRS_dev_ = getValue("GPRS_dev");
	//域名地址
	var GPRS_peer_ip = getValue("GPRS_peer_ip");
	//域名端口
	var GPRS_peer_port = getValue("GPRS_peer_port");
	//上传间隔
	var GPRS_upload_interval_ = getValue("GPRS_upload_interval");

	//是否启用心跳包
	var GPRS_heartbeat = getChecked("GPRS_heartbeat");
	
	//协议
	var GPRS_proto = getValue("GPRS_proto");
	var GPRS_proto_ms = getValue("GPRS_proto_ms");
	var GPRS_modbus_addr = getValue("GPRS_modbus_addr");
	
	var GPRS_wm_xfer_proto_type = getValue("GPRS_wm_xfer_proto_type");
	var GPRS_xfer_gw_trt_dst_type = getValue("GPRS_xfer_gw_trt_dst_type");
	var GPRS_xfer_gw_trt_dst_uart_type = getValue("GPRS_xfer_gw_trt_dst_uart_type");
	var GPRS_xfer_gw_trt_dst_uart_baudrate = getValue("GPRS_xfer_gw_trt_dst_uart_baudrate");
	var GPRS_xfer_gw_trt_dst_uart_bits = getValue("GPRS_xfer_gw_trt_dst_uart_bits");
	var GPRS_xfer_gw_trt_dst_net_index = getValue("GPRS_xfer_gw_trt_dst_net_index");

	var md = Number(GPRS_working_type);

	if( GPRS_group_is ) {
		if (chk2(GPRS_socket_type, GPRS_working_cs, "套接字类型")) return;
		//if (chk(GPRS_dev_, "设备NM号")) return;
		if( Number(GPRS_working_cs) != 1 ) {
			if (chk(GPRS_peer_ip, "域名地址")) return;
		}
		if (chk(GPRS_peer_port, "域名端口")) return;
		if (chk(GPRS_upload_interval_, "上传间隔")) return;
		//if (chk(GPRS_parameter, "上传的数据")) return;
		if(md == TCP_IP_M_NORMAL) {
			if(Number(GPRS_proto) == 1 ) {
				GPRS_proto_ms = 0;
			}
			if (chk2(GPRS_proto, GPRS_proto_ms, "协议")) return;
			if( Number(GPRS_proto)==PROTO_MODBUS_RTU_OVER_TCP&&Number(GPRS_proto_ms)==0) {
				if (chk(GPRS_modbus_addr, "Modbus 从机地址")) return;
			}
		} else if(md == TCP_IP_M_XFER) {
			var is_gw = getNumber('GPRS_xfer_mode')== XFER_M_GW;
			var is_trt = getNumber('GPRS_xfer_mode')== XFER_M_TRT;
			var dst_type = -1;
			if(is_gw||is_trt) {
				dst_type = getNumber('GPRS_xfer_gw_trt_dst_type');
			}
			if((is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX) {
				//if(chk(GPRS_xfer_gw_trt_dst_uart_type, "转发端口")) return;
				if(chk(GPRS_xfer_gw_trt_dst_uart_baudrate, "转发波特率")) return;
				if(chk(GPRS_xfer_gw_trt_dst_uart_bits, "转发数据位")) return;
			}
			if((is_gw||is_trt)&&dst_type==PROTO_DEV_NET) {
				if(chk(GPRS_xfer_gw_trt_dst_net_index, "转发目标以太网组号")) return;
			}
			if(is_gw||is_trt) {
				if(chk(GPRS_xfer_gw_trt_dst_type, "转发端口")) return;
			}
			if(is_gw) {
				if(chk(GPRS_wm_xfer_proto_type, "转发协议")) return;
			}
		}
	}

	//注：在此处填写保存代码即可；
	var val = getNumber("GPRS_group") + ENET_TCPIP_NUM;
	var setval = {
		n:Number(val), 
		en:GPRS_group_is?1:0, 
		md:Number(GPRS_working_type), 
		tt:Number(GPRS_socket_type), 
		cs:Number(GPRS_working_cs), 
		pe:GPRS_peer_ip, 
		po:Number(GPRS_peer_port), 
		it:Number(GPRS_upload_interval_), 
		kl:GPRS_heartbeat?1:0
	};
	
	if(md == TCP_IP_M_NORMAL) {
		setval['normal'] = {
			pt:Number(GPRS_proto), 
			ms:Number(GPRS_proto_ms), 
			mad:Number(GPRS_modbus_addr)
		}
	} else if(md == TCP_IP_M_XFER) {
		setval['xfer'] = {
			md:getNumber('GPRS_xfer_mode'), 
			pt:Number(GPRS_wm_xfer_proto_type), 
			dt:Number(GPRS_xfer_gw_trt_dst_type), 
			tidx:Number(GPRS_xfer_gw_trt_dst_net_index), 
			ubr:Number(GPRS_xfer_gw_trt_dst_uart_baudrate), 
			udb:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(0)), 
			upy:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(1)), 
			usb:parseInt(GPRS_xfer_gw_trt_dst_uart_bits.charAt(2))
		}
	}

	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + " TCP/IP配置","/cgi-bin/setTcpipCfg?",JSON.stringify(setval));
}

//GPRS 工作参数配置
function Show_GPRS_Config()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	MyGetJSONWithArg("正在获取" + GPRS_OR_NBIOT + "工作参数","/cgi-bin/getGPRSWorkCfg?","", function (cfg)
	{
		Page_GPRS_Config(cfg)
	});
}

function gprs_wmode_change()
{
	var _en = (getNumber('GPRS_Config_Mode') != 3);
	setEnable('GPRS_Config_Activation', _en);
	setEnable('GPRS_Config_Level', _en);
	setEnable('GPRS_Config_SIM', _en);
	setEnable('GPRS_Config_MS', _en);
	setEnable('GPRS_Config_Reg', _en);
	setEnable('GPRS_Config_Jump', _en);
	setEnable('GPRS_Config_Time', _en);
}

function Page_GPRS_Config(cfg)
{
	//工作模式
	setValue('GPRS_Config_Mode', cfg.wm);
	//激活方式
	setValue('GPRS_Config_Activation', cfg.om);
	//调试等级
	setValue('GPRS_Config_Level', cfg.dl);
	//SIM卡号码
	setValue('GPRS_Config_SIM', cfg.simno);
	//数据帧时间
	setValue('GPRS_Config_MS', cfg.it);
	//自定义注册包
	setValue('GPRS_Config_Reg', cfg.reg);
	//自定义心跳包
	setValue('GPRS_Config_Jump', cfg.hrt);
	//重连次数
	setValue('GPRS_Config_Time', cfg.rt);

	gprs_wmode_change();
}


function Apply_GPRS_Config()
{
	//工作模式
	var GPRS_Config_Mode = getValue("GPRS_Config_Mode");
	
	//激活方式
	var GPRS_Config_Activation = getValue("GPRS_Config_Activation");
	
	//调试等级
	var GPRS_Config_Level = getValue("GPRS_Config_Level");
	
	//SIM卡号码
	var GPRS_Config_SIM = getValue("GPRS_Config_SIM");
	//数据帧时间
	var GPRS_Config_MS = getValue("GPRS_Config_MS");
	//自定义注册包
	var GPRS_Config_Reg = getValue("GPRS_Config_Reg");
	//自定义心跳包
	var GPRS_Config_Jump = getValue("GPRS_Config_Jump");
	//重连次数
	var GPRS_Config_Time = getNumber("GPRS_Config_Time");
	
	//注：在此处填写保存代码即可；
	var setval = { };

	if( Number(GPRS_Config_Mode) != 3 ) {
		if (chk(GPRS_Config_Mode, "工作模式")) return;
		if (chk(GPRS_Config_Activation, "激活方式")) return;
		if (chk(GPRS_Config_Level, "调试等级")) return;
		//if (chk(GPRS_Config_SIM, "SIM卡号码")) return;
		if (chk(GPRS_Config_MS, "数据帧时间")) return;
		//if (chk(GPRS_Config_Reg, "自定义注册包")) return;
		//if (chk(GPRS_Config_Jump, "自定义心跳包")) return;
		if (chk(GPRS_Config_Time, "重连次数")) return;
		
		setval = {
			wm:Number(GPRS_Config_Mode), 
			om:Number(GPRS_Config_Activation), 
			dl:Number(GPRS_Config_Level), 
			simno:GPRS_Config_SIM, 
			it:Number(GPRS_Config_MS), 
			rt:Number(GPRS_Config_Time), 
			reg:GPRS_Config_Reg, 
			hrt:GPRS_Config_Jump
		};
		
	} else {
		setval = {
			wm:Number(GPRS_Config_Mode)
		}
	}

	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "工作参数","/cgi-bin/setGPRSWorkCfg?",JSON.stringify(setval));
}


//无线配置
function Show_GPRS_Wireless()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_GPRS_Wireless("注：此处传入一个对象");
	
	MyGetJSONWithArg("正在获取" + GPRS_OR_NBIOT + "配置","/cgi-bin/getGPRSNetCfg?","", function (info)
	{
		Page_GPRS_Wireless(info)
	});
}

function Page_GPRS_Wireless(info)
{
	//无线网络APN
	setValue('GPRS_Wireless_APN', info.apn);
	//APN用户名
	setValue('GPRS_Wireless_User', info.user);
	//APN密码
	setValue('GPRS_Wireless_PWD', info.psk);
	//APN拨号号码
	setValue('GPRS_Wireless_NUM', info.apnno);
	//短信中心号码
	setValue('GPRS_Wireless_Center', info.msgno);
}


function Apply_GPRS_Wireless()
{
	//无线网络APN
	var GPRS_Wireless_APN = getValue("GPRS_Wireless_APN");
	//APN用户名
	var GPRS_Wireless_User = getValue("GPRS_Wireless_User");
	//APN密码
	var GPRS_Wireless_PWD = getValue("GPRS_Wireless_PWD");
	//APN拨号号码
	var GPRS_Wireless_NUM = getValue("GPRS_Wireless_NUM");
	//短信中心号码
	var GPRS_Wireless_Center = getValue("GPRS_Wireless_Center");

	//if (chk(GPRS_Wireless_APN, "无线网络APN")) return;
	//if (chk(GPRS_Wireless_User, "APN用户名")) return;
	//if (chk(GPRS_Wireless_PWD, "APN密码")) return;
	//if (chk(GPRS_Wireless_NUM, "APN拨号号码")) return;
	//if (chk(GPRS_Wireless_Center, "短信中心号码")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		apn:GPRS_Wireless_APN, 
		user:GPRS_Wireless_User, 
		psk:GPRS_Wireless_PWD, 
		apnno:''+GPRS_Wireless_NUM, 
		msgno:''+GPRS_Wireless_Center
	};
	
	MyGetJSONWithArg("正在设置" + GPRS_OR_NBIOT + "配置信息","/cgi-bin/setGPRSNetCfg?",JSON.stringify(setval));
}


//串口配置
function Show_Serial_Port()
{
	var n = document.getElementById("Serial_Port_Select").selectedIndex;
	//var Serial_Port_Select = document.getElementById("Serial_Port_Select").options[v].text;
	//alert('当前选择【' + Serial_Port_Select + '】请此处添加代码。');
	//【你需要添加代码】通过“Serial_Port_Select”序号查找到对应的信息；
	//再把“对应信息”传给Page_Remote_Transmission函数；
	//Page_Serial_Port(Serial_Port_Select);
		
	var setval = {
		n:Number(n)
	};
	
	MyGetJSONWithArg("正在获取串口配置信息","/cgi-bin/getUartCfg?",JSON.stringify(setval), function( cfg ){
		Page_Serial_Port(cfg)
	});
}

function resreshLuaSelectList(_id, _list)
{
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	selectobj.options.length = 0;
	if (_list!=null&&_list.length>0) {
		for(var i = 0; i < _list.length; i++) {
			if (_list[i].type == 'file') {
				var _name = getLuaName(_list[i].name);
				if (_name.toLowerCase() != 'rtu_main') {
					selectobj.options.add(new Option(_name, _name));
				}
			}
		}
	}
}

function tryResreshLuaSelectList(_id)
{
	var selectobj = window.document.getElementById(_id);
	if(selectobj==null) return ;
	if (xLuaList==null||xLuaList.length<=0) {
		MyGetJSON("正在获取脚本列表","/cgi-bin/listDir?", 'path', "/lua" ,function( info ){
		if (info.list!=null && info.list!=null) {
			xLuaList = info.list.concat();
			for( var i = 0; i < xLuaList.length; i++ ) {
				if( xLuaList[i].type == 'file' && getLuaName(xLuaList[i].name).toLowerCase() == "rtu_main" ) {
					xLuaList[0] = xLuaList.splice(i,1,xLuaList[0])[0];
					break;
				}
			}
			resreshLuaSelectList(_id, info.list);
		}
		});
	} else {
		resreshLuaSelectList(_id,xLuaList);
	}
}

function onSerialProtoChange()
{
	setDisplay('Serial_Port_proto_lua_ul', getNumber('Serial_Port_proto') == PROTO_LUA );
	setDisplay('Serial_Port_proto_nlua_ul', getNumber('Serial_Port_proto') != PROTO_LUA && !__proto_is_master_fixed(getNumber('Serial_Port_proto')));
	if(getNumber('Serial_Port_proto') == PROTO_LUA) {
		tryResreshLuaSelectList('Serial_Port_proto_lua_list');
	}
	setDisplay('Serial_Port_proto_ms_addr', (getNumber('Serial_Port_proto_ms') == 0)&&(getNumber('Serial_Port_proto')!=PROTO_LUA)&&(!__proto_is_master_fixed(getNumber('Serial_Port_proto'))) );
}

function Page_Serial_Port(cfg)
{
	//串口模式
	setValue("Serial_Port_Mode", cfg.ut);

	//协议配置
	setValue("Serial_Port_proto", cfg.po);
	setValue("Serial_Port_proto_ms", cfg.ms);
	//mdobus从机地址
	setValue("Serial_Port_ModbusAddr", cfg.ad);
	
	//串口波特率
	setValue("Serial_Port_Rate", cfg.bd);
	//串口校验位
	//setValue("Serial_Port_parity", cfg.py);

	//采集间隔（MS）
	//change name by chenqianqian
	//setValue("Serial_Port_Interval", cfg.in);
	setValue("Serial_Port_Interval", cfg['in']);

	//数据位+校验位+停止位
	setValue("Serial_Port_Data", (Number(cfg.da) * 100) + (Number(cfg.py) * 10) + cfg.st );
	setDisplay('Serial_Port_proto_ms_addr', (getNumber('Serial_Port_proto_ms') == 0)&&(getNumber('Serial_Port_proto')!=PROTO_LUA)&&(!__proto_is_master_fixed(getNumber('Serial_Port_proto'))) );
	onSerialProtoChange(cfg.luapo);
	setValue("Serial_Port_proto_lua_list", cfg.luapo);
}

function Apply_Serial_Port()
{
	//选择串口
	var Serial_Port_Select = document.getElementById("Serial_Port_Select").value;
	
	//var Serial_Port_Mode = document.getElementById("Serial_Port_Mode").value;
	
	//协议配置
	var Serial_Port_proto = document.getElementById("Serial_Port_proto").value;
	var Serial_Port_proto_lua = document.getElementById("Serial_Port_proto_lua_list").value;
	var Serial_Port_proto_ms = document.getElementById("Serial_Port_proto_ms").value;
		
	//mdobus从机地址
	var Serial_Port_ModbusAddr = document.getElementById("Serial_Port_ModbusAddr").value;
	
	//串口波特率
	var Serial_Port_Rate = document.getElementById("Serial_Port_Rate").value;
	//var Serial_Port_parity = document.getElementById("Serial_Port_parity").value;

	//采集间隔（MS）
	var Serial_Port_Interval = document.getElementById("Serial_Port_Interval").value;
	
	//数据位
	var Serial_Port_data_stop = document.getElementById("Serial_Port_Data").value;

	if (chk(Serial_Port_Select, "选择串口")) return;
	//if (chk(Serial_Port_Mode, "选择串口模式")) return;
	if (chk(Serial_Port_proto, "协议配置")) return;
	if(Number(Serial_Port_proto)==PROTO_LUA) {
		if (chk(Serial_Port_proto_lua, "选择lua协议")) return;
	} else {
		if (chk(Serial_Port_proto_ms, "协议主从配置")) return;
		if( Number(Serial_Port_proto_ms) ==  0 ) {
			if (chk(Serial_Port_ModbusAddr, "Modbus从机地址")) return;
		}
	}
	if (chk(Serial_Port_Rate, "串口波特率")) return;

	//if (chk(Serial_Port_parity, "校验位")) return;
	if (chk(Serial_Port_Interval, "采集间隔")) return;
	
	var Serial_Port_data = parseInt(Serial_Port_data_stop.charAt(0));
	var Serial_Port_parity = parseInt(Serial_Port_data_stop.charAt(1));
	var Serial_Port_stop = parseInt(Serial_Port_data_stop.charAt(2));

	//注：在此处填写保存代码即可；
	var setval = {
		n:Number(Serial_Port_Select), 
		bd:Number(Serial_Port_Rate), 
		//ut:Number(Serial_Port_Mode), 
		po:Number(Serial_Port_proto), 
		py:Number(Serial_Port_parity), 
		ms:Number(Serial_Port_proto_ms), 
		luapo:Serial_Port_proto_lua,
		ad:Number(Serial_Port_ModbusAddr), 
		//change name by chenqq
		//in:Number(Serial_Port_Interval), 
		'in':Number(Serial_Port_Interval), 
		da:Number(Serial_Port_data), 
		st:Number(Serial_Port_stop)
	};
	
	MyGetJSONWithArg("正在设置串口配置信息","/cgi-bin/setUartCfg?",JSON.stringify(setval));
}

//开关量输入设置
function Show_Switch_Cfg()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Switch("注：此处传入一个对象");
	
	MyGetJSONWithArg("正在获取输入模块配置","/cgi-bin/getDiCfg?","", function( info ){
		Page_Switch_Cfg(info.cfg);
	});
}

function Page_Switch_Cfg(cfg)
{
	for(var i = 0; i < 8; i++) {
		document.getElementsByName("road_turnno")[i].checked = Number(cfg[i].en!=0);
	   // setValue("switch_interval"+(i+1), cfg[i].in);
	   // setValue("switch_expression"+(i+1), cfg[i].exp);
	   setValue("switch_interval"+(i+1), cfg[i]['in']);
	   setValue("switch_expression"+(i+1), cfg[i]['exp']); 
	}

}

function Show_Switch_Value()
{
	MyGetJSONWithArg("","/cgi-bin/getDiValue?","", function( info ){
		Page_Switch_Value(info.di);
	});
}

function Page_Switch_Value(di)
{
	for(var i = 0; i < 8; i++) {
		document.getElementById("switch_state"+(i+1)).innerHTML = Number(di[i].va!=0) ? "<font color='Green'>【高】</font>" : "<font color='Green'>【低】</font>";
	}	
	/*for(var i = 4; i < 8; i++) {
		document.getElementById("switch_state"+(i+1)).innerHTML = Number(di[i].va!=0) ? "<font color='Green'>【打开】</font>" : "<font color='Green'>【关闭】</font>";
	}*/
}

function Apply_Switch()
{
	var inputset = {
		cfg:new Array()
	};
	
	for(var i = 0; i < 8; i++) {
		inputset.cfg[i] = { 
			n:i, 
			en:(document.getElementsByName("road_turnno")[i].checked?1:0), 
		   // in:getNumber("switch_interval"+(i+1)),
		   'in': getNumber("switch_interval"+(i+1)),
			'exp':getValue("switch_expression"+(i+1))
		};
		
		if( inputset.cfg[i].exp != null && inputset.cfg[i].exp.length > 128 ) {
			alert("表达式长度不能超过128字节");
		}
	}

	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在设置输入模块配置","/cgi-bin/setDiCfg?",JSON.stringify(inputset));
}


//开关量输出设置
function Show_Output_Cfg()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Output("注：此处传入一个对象");
	MyGetJSONWithArg("正在获取输出模块配置","/cgi-bin/getDoCfg?","",function( info ){
		Page_Output_Cfg(info.cfg);
	});
}

function Page_Output_Cfg(cfg)
{	
	for(var i = 0; i < 6; i++) {
		//change name by chenqq
		//setValue( "Output_Expression"+(i+1), cfg[i]._exp );
		
		//change mode by jay
		setValue( "Output_Expression"+(i+1), cfg[i]['exp']);
	}
}

function Show_Output_Value()
{
	//*注：在此处编写获取代码；然而调用如下函数，实现页面显示；
	//Page_Output("注：此处传入一个对象");
	MyGetJSONWithArg("正在获取输出模块信息","/cgi-bin/getDoValue?","",function( info ){
		Page_Output_Value(info['do']);
	});
}

function Page_Output_Value(_do)
{
	for(var i = 0; i < 4; i++) {
		setValue("Output_State"+(i+1), 0==Number(_do[i].va)?"开":"关");
	}
	
	for(var i = 4; i < 6; i++) {
		setValue("Output_State"+(i+1), 0==Number(_do[i].va)?"低":"高");
	}
}

function OutPutClick(n)
{	
	var OutputSet = {
		'do':new Array()
	};
	n=n-1;
	if( n < 4 ) {
		OutputSet['do'][n] = { n:n, va:(getValue("Output_State"+(n+1))=="开"?1:0) };
	} else {
		OutputSet['do'][n] = { n:n, va:(getValue("Output_State"+(n+1))=="高"?0:1) };
	}
		
	MyGetJSONWithArg("正在设置输出模块信息","/cgi-bin/setDoValue?",JSON.stringify(OutputSet),function( info ){
		Show_Output_Value();
	});
}

function Apply_Output()
{

	var onputset = {
		cfg:new Array()
	};
	for(var i = 0; i < 6; i++) {
		onputset.cfg[i] = { 
			n:i, 
			exp:(getValue("Output_Expression"+(i+1)))
		};		
		
		if( onputset.cfg[i].exp != null && onputset.cfg[i].exp.length > 128 ) {
			alert("表达式长度不能超过128字节");
		}
	}

	//注：在此处填写保存代码即可；
	MyGetJSONWithArg("正在设置输出模块信息","/cgi-bin/setDoCfg?",JSON.stringify(onputset));
}


//模拟量输入设置
function Show_Input()
{
	var n = document.getElementById("Input_Select").value;
	//var Input_Select = document.getElementById("Input_Select").options[v].text;
	//alert('当前选择【' + Input_Select + '】请此处添加代码。');
	//========================================================================>
	var setval = {
		n:Number(n)
	};
	
	MyGetJSONWithArg("正在获取模拟量输入配置","/cgi-bin/getAnalogCfg?", JSON.stringify(setval),function( cfg ){
		Page_Input(cfg);
	});
}

function Show_Input_Range_Electrical()
{
	//电参数实测值	
	var n = document.getElementById("Input_Select").value;
	//var Input_Select = document.getElementById("Input_Select").options[v].text;
	//alert('当前选择【' + Input_Select + '】请此处添加代码。');
	//========================================================================>
	var setval = {
		n:Number(n)
	};
	
	MyGetJSONWithArg("","/cgi-bin/getAnalogCfg?", JSON.stringify(setval),function( info ){
		setValue('Input_Range_Engineering', info.eg);
		setValue('Input_Range_Electrical', info.mv);
	});
}

function Enable_Input()
{
	var v = !getChecked("Input_Group_Is");

	document.getElementById("Input_Interval").disabled = v;
	document.getElementById("Input_Format").disabled = v;
	document.getElementById("Input_Range").disabled = v;
	document.getElementById("Input_Range_Engineering").disabled = v;

	document.getElementById("Input_Range_Electrical").disabled = v;
	document.getElementById("Input_Range_Max").disabled = v;
	document.getElementById("Input_Range_Min").disabled = v;
	document.getElementById("Input_Correction_Factor").disabled = v;
	//document.getElementById("Input_Expression").disabled = v;
	
}

function Page_Input(cfg)
{
	//是否启用
	setCheckBoxEnable("Input_Group_Is", Number(cfg.en)!=0);
    
    Enable_Input();

	//采集间隔
	setValue('Input_Interval', cfg.it);

	//数据格式
	setValue("Input_Format", cfg.ut );
	
	//量程设置
	setValue("Input_Range", cfg.rg);

	//工程量实测值
	setValue('Input_Range_Engineering', cfg.eg);
	//电参数实测值
	setValue('Input_Range_Electrical', cfg.mv);

	//最大量程
	setValue('Input_Range_Max', cfg.ea);
	//最小量程
	setValue('Input_Range_Min', cfg.ei);
	//修正系数
	setValue('Input_Correction_Factor', cfg.ec);
	//表达式
	//setValue('Input_Expression', obj.null);
	
	Hide_Input();
}

function Hide_Input()
{
	var n = Number(document.getElementById("Input_Format").value);
	if ( 3 == n )
	{
		document.getElementById("Input_Show_Max").style.display = "";
		document.getElementById("Input_Show_Min").style.display = "";
		document.getElementById("Input_Show_Factor").style.display = "";
	}
	else
	{
		document.getElementById("Input_Show_Max").style.display = "none";
		document.getElementById("Input_Show_Min").style.display = "none";
		document.getElementById("Input_Show_Factor").style.display = "none";
	}
}

function Apply_Input()
{
	var Input_Select = document.getElementById("Input_Select").value;
	//是否启用
	var Input_Group_Is = getChecked("Input_Group_Is");
	
	//采集间隔
	var Input_Interval = document.getElementById("Input_Interval").value;
	//数据格式
	var Input_Format = document.getElementById("Input_Format").value;
	//量程设置
	var Input_Range = document.getElementById("Input_Range").value;
	//最大量程
	var Input_Range_Max = document.getElementById("Input_Range_Max").value;
	//最小量程
	var Input_Range_Min = document.getElementById("Input_Range_Min").value;
	//修正系数
	var Input_Correction_Factor = document.getElementById("Input_Correction_Factor").value;
	//表达式
	//var Input_Expression = document.getElementById("Input_Expression").value;

	if( Input_Group_Is ) {
		if (chk(Input_Interval, "采集间隔")) return;
		if (chk(Input_Format, "数据格式")) return;
		if (chk(Input_Range, "量程设置")) return;

		if ( 3 == Input_Format )
		{
			if (chk(Input_Range_Max, "最大量程")) return;
			if (chk(Input_Range_Min, "最小量程")) return;
			if (chk(Input_Correction_Factor, "修正系数")) return;
		}
	}

	//if (chk(Input_Expression, "表达式")) return;

	//注：在此处填写保存代码即可；
	var setval = {
		n:Number(Input_Select), 
		en:Input_Group_Is?1:0, 
		it:Number(Input_Interval), 
		rg:Number(Input_Range), 
		ut:Number(Input_Format), 
		ei:Number(Input_Range_Min), 
		ea:Number(Input_Range_Max), 
		ec:Number(Input_Correction_Factor)
	};	
	
	
	MyGetJSONWithArg("正在设置模拟量输入配置","/cgi-bin/setAnalogCfg?",JSON.stringify(setval));
}

function getLuaName(fname)
{
	var n = fname.toLowerCase().lastIndexOf(".lua");
	if(n>0) return fname.substring(0,n);
	return fname;
}

function checkLua(luaname)
{
	for(var i = 0; i < xLuaList.length; i++) {
		if (xLuaList[i].type == 'file') {
			if (getLuaName(xLuaList[i].name).toLowerCase() == luaname.toLowerCase()) {
				return false;
			}
		}
	}
	return true;
}

function Show_LuaScript()
{
	MyGetJSON("正在获取脚本列表","/cgi-bin/listDir?", 'path', "/lua" ,function( info ){
		if (info.list!=null && info.list!=null) {
			xLuaList = info.list.concat();
			for( var i = 0; i < xLuaList.length; i++ ) {
				if( xLuaList[i].type == 'file' && getLuaName(xLuaList[i].name).toLowerCase() == "rtu_main" ) {
					xLuaList[0] = xLuaList.splice(i,1,xLuaList[0])[0];
					break;
				}
			}
			myTableItemRemoveAll('luascript_table');
			for(var i = 0; i < xLuaList.length; i++) {
				if (xLuaList[i].type == 'file') {
					luaScriptTableAddItem(getLuaName(xLuaList[i].name),xLuaList[i].size,xLuaList[i].mtime);
				}
			}
		}
	});
}

function onLuaScriptTableItemDbClick(table, row)
{	
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
		for (var i = 0; i < table.rows.length; i++) {
			table.rows[i].style.background="#FFFFFF";
		}
		row.style.background="#E5E5E5";
		
		var luascript_title = _tostr_(row.cells[0].innerHTML);
		$.ajax({
			type: "get",
			url: "/download/media/nand/lua/"+getLuaName(luascript_title)+".lua",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				setValue('luascript_title', luascript_title);
				setValue('luascript_content', _tostr_(data));
				setEnable("btn_del", true);
			}
		});
	}
}

function luaScriptTableAddItem(fname,fsize,mtime)
{
	var table = window.document.getElementById("luascript_table");
	var row = table.insertRow(table.rows.length);
	row.ondblclick = function(){ onLuaScriptTableItemDbClick( table, row ); };
	obj = row.insertCell(0);
	obj.innerHTML = _tostr_(fname);
	var obj = row.insertCell(1);
	obj.innerHTML = _tostr_(fsize);
	obj = row.insertCell(2);
	obj.innerHTML = _tostr_(mtime);
}

//新建
function New_LuaScript()
{
	setValue('luascript_title', '');
	setValue('luascript_content', '');
	setEnable("btn_del", false);
}

//保存
function Save_LuaScript()
{
	var luascript_title = document.getElementById("luascript_title").value;
	if (chk(luascript_title, "脚本名称")) return;
	
	var msg = checkLua(luascript_title)?'确定继续保存？':'警告：脚本已存在，确定覆写？';
	if (confirm(msg)){
		var luascript_content = document.getElementById("luascript_content").value;
		
		if (chk(luascript_content, "脚本内容")) return;
		
		var xhr = createXMLHttpRequest();
		if (xhr) {
			Show("正在保存脚本,请稍后...");
			xhr.onreadystatechange = function() {
				if( xhr.readyState==4 ) {
					Close();
					if( xhr.status==200 ) {
						alert("保存脚本成功！");
						Show_LuaScript();
						Show_LuaScript();
					} else {
						alert("保存失败,请重试！");
					}
				}
			}
			xhr.open("POST", "/lua/upload/media/nand/lua/"+luascript_title+".lua" );
			xhr.send(luascript_content);
		}
	}
}

function Del_LuaScript()
{
	if (confirm('你确定继续删除？'))
	{
		//=======================删除接口
	}
}

//导出
function Derived(){
	if (confirm('你确定导出吗？')){
		var Derived_type = document.getElementById("Derived_type").value;
		var data_export=document.getElementById("data_export").value;
		//console.log($('#Derived_type').val());
		if (chk(data_export, "数据导出")) return;
		if (chk(Derived_type, "导出类型")) return;
	}
}

function Show_Defin()
{
	var n = document.getElementById("div_defin_script");
	//========================================================================
	//获取全部脚本列表；（不需分页）
	//========================================================================
	/*var s = "";
	s += "<table style='width:96%;margin-top:0;'>";
	s+='<tr><td class="back_8fc6">ID</td><td class="back_8fc6">变量名</td><td class="back_8fc6">数据类型</td><td class="back_8fc6">初始值</td><td class="back_8fc6">最小值</td><td class="back_8fc6">最大值</td><td class="back_8fc6">表达式</td><td class="back_8fc6">别名</td><td class="back_8fc6">备注说明</td><td class="back_8fc6">操作</td></tr>';
	//------------重复-----------------
	s += "<tr><td>1</td><td>prame1</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率1</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"1\",\"prame1\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率1\");' /></td></tr>";
	s += "<tr><td>2</td><td>prame2</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率2</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"2\",\"prame2\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率2\");' /></td></tr>";
	s += "<tr><td>3</td><td>prame3</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率3</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"3\",\"prame3\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率3\");' /></td></tr>";
	s += "<tr><td>4</td><td>prame4</td><td>Int</td><td>-1</td><td>1</td><td>99</td><td>x(a,b){a * 100 * b};</td><td>功率</td><td>该参数关于设备释放功率4</td><td><input type='button' value='查看' class='b_s' onclick='Detail_Defin(\"4\",\"prame4\",\"0\",\"-1\",\"1\",\"99\",\"x(a,b){a * 100 * b};\",\"功率\",\"该参数关于设备释放功率4\");' /></td></tr>";
	//---------------------------------
	s += "</table>";

	n.innerHTML = s;*/
}


function Detail_Defin(a, b, c, d, e, f, g, h, i)
{
	document.getElementById("user_defin_id").value = a;
	document.getElementById("user_defin_name").value = b;
	setValue('user_defin_datatype', c);
	document.getElementById("user_defin_init").value = d;
	document.getElementById("user_defin_min").value = e;
	document.getElementById("user_defin_max").value = f;
	document.getElementById("user_defin_express").value = g;
	document.getElementById("user_defin_rename").value = h;
	document.getElementById("user_defin_note").value = i;

	setEnable("btn_defin_del", true);
}

//新建
function Clear_Defin()
{
	document.getElementById("user_defin_id").value = "";
	document.getElementById("user_defin_name").value = "";
	setValue('user_defin_datatype', 0);
	document.getElementById("user_defin_init").value = "";
	document.getElementById("user_defin_min").value = "";
	document.getElementById("user_defin_max").value = "";
	document.getElementById("user_defin_express").value = "";
	document.getElementById("user_defin_rename").value = "";
	document.getElementById("user_defin_note").value = "";
	setEnable("btn_defin_del", false);
}


//保存
function Add_Defin()
{
	if (confirm('你确定继续保存？'))
	{
		var n = document.getElementById("div_defin_script");
		var user_defin_id = document.getElementById("user_defin_id").value;
		var user_defin_name = document.getElementById("user_defin_name").value;
		var user_defin_datatype = document.getElementById("user_defin_datatype").value;
		var user_defin_init = document.getElementById("user_defin_init").value;
		var user_defin_min = document.getElementById("user_defin_min").value;
		var user_defin_max = document.getElementById("user_defin_max").value;
		var user_defin_express = document.getElementById("user_defin_express").value;
		var user_defin_rename = document.getElementById("user_defin_rename").value;
		var user_defin_note = document.getElementById("user_defin_note").value;

		if (chk(user_defin_id, "ID")) return;
		if (chk(user_defin_name, "变量名")) return;
		if (chk(user_defin_datatype, "数据类型")) return;
		if (chk(user_defin_init, "初始值")) return;
		if (chk(user_defin_min, "最小值")) return;
		if (chk(user_defin_max, "最大值")) return;
		if (chk(user_defin_express, "表达式")) return;
		if (chk(user_defin_rename, "别名")) return;
		if (chk(user_defin_note, "备注说明")) return;

		if (n.innerHTML.lastIndexOf("<td>" + user_defin_name + "</td>") == -1)
		{
			//=======================保存接口 
			alert('保存成功');
			Show_Defin();
			Clear_Defin();
		}
		else
		{
			alert("变量名在RTU设备已存在！！");
		}
	}
}

function Del_Defin()
{
	if (confirm('你确定继续删除？'))
	{
		//=======================删除接口
	}
}

function resetdev()
{
	if( window.confirm('确定要重启设备吗?\n\n重启大约需要40秒钟') ){
		MyGetJSONWithArg("","/cgi-bin/devReset?","",function( cfg ){
			location.reload(true);
		});
	}
}
/*
function downloadCfg()
{
	window.open("http://"+window.location.host+"/download/" + CONFIG_PATH + "/rtu_cfg_v0.cfg");
}
*/

function downloadCfg()
{
	MyGetJSONWithArg("","/cgi-bin/saveCfgWithJson?","",function(res){
		if( res != null && 0 == res.ret ) {
			window.open("http://"+window.location.host+"/download/" + CONFIG_PATH + "/rtu_board_json.cfg");
		} else {
			alert("获取配置失败，请检查后重试！");
		}
	});
	
}

function factoryReset()
{
	if( window.confirm('建议先备份配置.\n\n确定要恢复出厂设置吗?\n\n') ){
		MyGetJSONWithArg("","/cgi-bin/factoryReset?","",function( cfg ){
			location.reload(true);
		});
	}
}

//上传数据配置 chenqq

function Show_upload_cfg(){
	//列表
	MyGetJSONWithArg("正在获取数据上传配置,请稍后...","/cgi-bin/getVarManageExtData?", "{\"all\":1}",function (res) {
		if( res != null && 0 == res.ret ) {
			xVarManageExtDataBase = res.list.concat();
			xProtoDevList = res.protolist;
			xUpProtoDevList = res.upprotolist;
			refreshAllUploadDataCfgBase(0);
		} else {
			alert("获取失败,请重试");
		}
	});
}

function refreshAllUploadDataCfgBase(index)
{
	refreshProtoDevList( xUpProtoDevList, "undefine____dead____", "upload_data_pro_dev" );
	myTableItemRemoveAll("upload_data_table");
	for( var n = 0; n < xVarManageExtDataBase.length; n++ ) {
		var _var = xVarManageExtDataBase[n];
		var _io = _var['io'];
		var _up = _var['up'];
		
		upCfgTableAddItem( 
			_up.en, 
			_tostr_(_var.na), 
			varExtGetVarTypeName(_io.vt,_io.vs), 
			_tostr_(_up.nid), 
			_tostr_(_up.fid), 
			_tostr_(_up.unit), 
			varExtGetProtoDev(_up.dt, _up.dt == PROTO_DEV_GPRS ? _up.dtn - ENET_TCPIP_NUM : _up.dtn), 
			varExtGetProto(_up.dt,_up.pt),
			_tostr_(_up.desc)
		);
	}
	var table = window.document.getElementById("upload_data_table");
	onUpCfgTableItemClick(table, table.rows[index]);
}

function upCfgTableAddItem(enable,name,valtype,nid,fid,unit,podev,potype,desc)
{		

	var table = window.document.getElementById("upload_data_table");
	var row = table.insertRow(table.rows.length);
	row.style.height="25px";
	row.onclick = function(){onUpCfgTableItemClick( table,row );}
	var obj = row.insertCell(0);
	obj.innerHTML = enable!=0?"启用":"停用";
	obj = row.insertCell(1);
	obj.innerHTML = name;
	obj = row.insertCell(2);
	obj.innerHTML = valtype;
	obj = row.insertCell(3);
	obj.innerHTML = nid;
	obj = row.insertCell(4);
	obj.innerHTML = fid;
	obj = row.insertCell(5);
	obj.innerHTML = unit;
	obj = row.insertCell(6);
	obj.innerHTML = enable!=0?podev:'--';
	obj = row.insertCell(7);
	obj.innerHTML = enable!=0?potype:'----';
	obj = row.insertCell(8);
	obj.innerHTML = desc;
}

function onUpCfgTableItemClick(tb,row)
{
	if( row != null && row.rowIndex != null && row.rowIndex >= 0 ) {
		
		for (var i = 0; i < tb.rows.length; i++) {
			if( xVarManageExtDataBase[i]['up'].en > 0 ) {
				tb.rows[i].style.background="#FFFFFF";
			} else {
				tb.rows[i].style.background="#F0F0F0";
			}
		}
		row.style.background="#E5E5E5";
		
		var _rowIndex = row.rowIndex;//去掉表头
		var _var = xVarManageExtDataBase[_rowIndex];
		var _io = _var['io'];
		var _up = _var['up'];
		if( _var!=null && _io!=null && _up!=null  ) {
			setCheckBoxEnable('upload_data_enable', _up.en);   //启用   （只或取这个已启用，下面两内容）
			setValue('upload_data_name', _var.na);	 //变量名
			setValue('upload_data_vartype', varExtGetVarTypeName( _io.vt, _io.vs));   //数据类型
			setValue('upload_data_nid', _up.nid); 
			setValue('upload_data_fid', _up.fid); 
			setValue('upload_data_unit', _up.unit);
			setValue('upload_data_pro_dev', _up.dt + "|" + _up.dtn);
			setValue('upload_data_pro_type', _up.pt);
			setValue('upload_data_desc', _up.desc); 
		}
		refreshOneProtoDevList(true,'upload_data_pro_dev','upload_data_pro_type');
	}
}

function setUpCfg()
{
	var _name = getValue('upload_data_name');
	var id = -1;
	
	for(var i = 0; i < xVarManageExtDataBase.length; i++) {
		if( _name == xVarManageExtDataBase[i].na ) {
			id = i;
			break;
		}
	}
	
	if( id >= 0 && id < EXT_VAR_LIMIT ) {
		var _upload_data_nid = getValue('upload_data_nid');
		var _upload_data_fid = getValue('upload_data_fid');
		var _upload_data_pro_dev = getValue('upload_data_pro_dev');
		var _upload_data_pro_type = getValue('upload_data_pro_type');
		
		//if (chk(_upload_data_nid, "关联Nid")) return;
		if (chk(_upload_data_fid, "关联Fid")) return;
		if (chk(_upload_data_pro_dev, "上传接口")) return;
		if (chk(_upload_data_pro_type, "上传协议")) return;
	
		var proto = getValue("upload_data_pro_dev").split("|");
		var _up = {
			en:getChecked('upload_data_enable')?1:0,
			nid:getValue('upload_data_nid'), 
			fid:getValue('upload_data_fid'), 
			unit:getValue('upload_data_unit'), 
			dt:Number(proto[0]), 
			dtn:Number(proto[1]), 
			pt:getNumber('upload_data_pro_type'), 
			desc:getValue('upload_data_desc')
		};
		var setval = {
			n:id, 
			'up':_up
		};
		
		if( xVarManageExtDataBase != null && id < xVarManageExtDataBase.length ) {
			if(xVarManageExtDataBase[id]['up']==null) xVarManageExtDataBase[id]['up'] = new Array();
			xVarManageExtDataBase[id]['up'].en=_up.en;
			xVarManageExtDataBase[id]['up'].nid=_up.nid;
			xVarManageExtDataBase[id]['up'].fid=_up.fid;
			xVarManageExtDataBase[id]['up'].unit=_up.unit;
			xVarManageExtDataBase[id]['up'].dt=_up.dt;
			xVarManageExtDataBase[id]['up'].dtn=_up.dtn;
			xVarManageExtDataBase[id]['up'].pt=_up.pt;
			xVarManageExtDataBase[id]['up'].desc=_up.desc;
		}
		
		MyGetJSONWithArg("正在设置采集变量信息,请稍后...","/cgi-bin/setVarManageExtData?",JSON.stringify(setval), function (res) {
			if( res != null && 0 == res.ret ) {
				getAllVarManageExtDataBase();
				refreshAllUploadDataCfgBase(id);
				alert( "设置成功" );
			} else {
				alert("设置失败,请重试");
			}
		});
	} else {
		alert("请先在列表中，选择要修改的选项，再进行设置");
	}
}

/**end 上传数据配置功能***/

function Page_Xfer_Net_Cfg(n,cfg)
{
	//是否启用
	document.getElementsByName("xfer_net_radio_"+n)[0].checked = (Number(cfg.en)!=0);
	document.getElementsByName("xfer_net_radio_"+n)[1].checked = !(Number(cfg.en)!=0);

	setValue('xfer_net_type_'+n, cfg.tt);
	setValue("xfer_net_cs_"+n, cfg.cs );
	setValue("xfer_net_proto_type_"+n, cfg.pt);
	setValue('xfer_net_peer_'+n, cfg.pe);
	setValue('xfer_net_port_'+n, cfg.po);
	setValue('xfer_net_dst_type_'+n, cfg.dt);
	
	if( cfg.uty != null ) setValue('xfer_net_dst_uart_type_'+n, cfg.uty);
	if( cfg.ubr != null ) setValue('xfer_net_dst_uart_baudrate_'+n, cfg.ubr);
	if( cfg.udb != null && cfg.usb != null  && cfg.upy != null ) setValue('xfer_net_dst_uart_bits_'+n, (Number(cfg.udb) * 100) + (Number(cfg.upy) * 10) + Number(cfg.usb));
	
	setDisplay('xfer_net_cs_'+n+'_ip', getNumber('xfer_net_cs_'+n) == 0 );
}

function setXferNetCfg(n)
{
	var xfer_net_radio_a = document.getElementsByName("xfer_net_radio_"+n)[0].checked;
	var xfer_net_radio_b = document.getElementsByName("xfer_net_radio_"+n)[1].checked;
	var xfer_net_type_ = getValue('xfer_net_type_'+n);
	var xfer_net_cs_ = getValue('xfer_net_cs_'+n);
	var xfer_net_proto_type_ = getValue('xfer_net_proto_type_'+n);
	var xfer_net_peer_ = getValue('xfer_net_peer_'+n);
	var xfer_net_port_ = getValue('xfer_net_port_'+n);
	var xfer_net_dst_type_ = getValue('xfer_net_dst_type_'+n);
	//var xfer_net_dst_uart_type_ = getValue('xfer_net_dst_uart_type_'+n);
	var xfer_net_dst_uart_baudrate_ = getValue('xfer_net_dst_uart_baudrate_'+n);
	var xfer_net_dst_uart_bits_ = getValue('xfer_net_dst_uart_bits_'+n);

	if (chk2(xfer_net_radio_a, xfer_net_radio_b, "是否启用")) return;
	if( xfer_net_radio_a ) {
		if (chk(xfer_net_type_, "工作模式")) return;
		if (chk(xfer_net_cs_, "工作模式")) return;
		if (chk(xfer_net_proto_type_, "转发协议")) return;
		if( Number(xfer_net_cs_) == 0 ) {
			if (chk(xfer_net_peer_, "IP地址或域名")) return;
		}
		if (chk(xfer_net_port_, "端口号")) return;
		if (chk(xfer_net_dst_type_, "转发接口")) return;
		if( Number(xfer_net_dst_type_) >= 0 && Number(xfer_net_dst_type_) <= 4 ) {
			//if (chk(xfer_net_dst_uart_type_, "串口类型")) return;
			if (chk(xfer_net_dst_uart_baudrate_, "串口波特率")) return;
			if (chk(xfer_net_dst_uart_bits_, "串口数据位")) return;
		}
	}

	//注：在此处填写保存代码即可；
	var setval = {
		n:Number(n), 
		en:xfer_net_radio_a?1:0, 
		tt:Number(xfer_net_type_), 
		cs:Number(xfer_net_cs_), 
		pt:Number(xfer_net_proto_type_), 
		pe:xfer_net_peer_, 
		po:Number(xfer_net_port_), 
		dt:Number(xfer_net_dst_type_), 
		//uty:Number(xfer_net_dst_uart_type_), 
		ubr:Number(xfer_net_dst_uart_baudrate_), 
		udb:parseInt(xfer_net_dst_uart_bits_.charAt(0)), 
		upy:parseInt(xfer_net_dst_uart_bits_.charAt(1)), 
		usb:parseInt(xfer_net_dst_uart_bits_.charAt(2))
	};	
	
	MyGetJSONWithArg("正在设置转发配置","/cgi-bin/setXferNetCfg?",JSON.stringify(setval));
}

function numToString(num, xx, len) {
	var str_xx = '0000000000000000'+ num.toString(xx);
	if(str_xx.length >= len) {
		return str_xx.substring(str_xx.length - len, str_xx.length);
	}
}

function stringArrayToUint8Array_check(strings) {
	var index = arguments[1]?arguments[1]:0;
	var stringList = strings.split(' ');
	for (var i = index; i < stringList.length; i++) {
		var num = parseInt(stringList[i], 16);
		if( isNaN(num) ) {
			return false;
		}
	}
	return true;
}

function stringArrayToUint8Array(strings) {
	var index = arguments[1]?arguments[1]:0;
	var stringList = strings.split(' ');
	var uintArray = [];
	for (var i = index; i < stringList.length; i++) {
		var num = parseInt(stringList[i], 16);
		if( isNaN(num) ) {
			alert("非法内容:" + stringList[i]);
			return null;
		}
		uintArray.push(num);
	}
	return new Uint8Array(uintArray);
}

function stringToUint8Array(string) {
	var index = arguments[1]?arguments[1]:0;
	var charList = string.split('');
	var uintArray = [];
	for (var i = index; i < charList.length; i++) {
		var wchr = charList[i].charCodeAt(0);
		if(wchr<0x80){
			uintArray.push(wchr);
		} else if(wchr<0x800 ) {
			var c1=wchr&0xff;
			var c2=(wchr>>8)&0xff;
			uintArray.push(0xC0|(c2<<2)|((c1>>6)&0x3));
			uintArray.push(0x80|(c1&0x3F));
		} else {
			var c1=wchr&0xff;
			var c2=(wchr>>8)&0xff;
			uintArray.push(0xE0|(c2>>4));
			uintArray.push(0x80|((c2<<2)&0x3C)|((c1>>6)&0x3));
			uintArray.push(0x80|(c1&0x3F));
		}
	}
	return new Uint8Array(uintArray);
}

function uint8ArrayToString(array) {
	var index = arguments[1]?arguments[1]:0;
	var out,i,len, c1,c2,c3;
	out="";len=array.length;i=index;
	while(i<len){
		c1 = array[i++];
		switch(c1>>4){
		case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7: out+=String.fromCharCode(c1); break;
		case 12:case 13: c2=array[i++];out+=String.fromCharCode(((c1&0x1F)<<6)|(c2&0x3F)); break;
		case 14:c2=array[i++];c3=array[i++];out+=String.fromCharCode(((c1&0x0F)<<12)|((c2&0x3F)<<6)|((c3&0x3F)<<0)); break;
		}
	}
	return out;
}

function uint8ArrayToHexString(uintArray) {
	var index = arguments[1]?arguments[1]:0;
	var string = "";
	for(var i=index; i<uintArray.length; i++) {
	   var tmp = uintArray[i].toString(16);
	   if(tmp.length == 1) {
		   tmp = "0" + tmp;
	   }
	   string += ((i==index)?tmp:(" " + tmp));
	}
	return string;
}

//add by chenqq 
function showDialog(id){
	applyVarExtInfo(1,0);
	refreshOneProtoDevList(false,'var_ext_pro_dev','var_ext_pro_type');
	$('.theme-popover-mask').show();
	$('#'+id).show();
}

function hideDialog(id){
	$('.theme-popover-mask').hide();
	$('#'+id).hide();
}

//采集变量删除操作
function Del_data_collection(){
	 var _val = $("#var_ext_name0").val();
	 MyGetJSONWithArg("正在删除采集变量","/cgi-bin/delVarManageExtData?", JSON.stringify({"na":_val}), function(){
		//重新请求列表
		getAllVarManageExtDataBase();
	 });
}

function remote_work_cs_change() {
	setDisplay('remote_working_cs_ip', getNumber('remote_working_cs') == 0 );
	setDisplay('remote_working_cs_interval', getNumber('remote_working_cs') == 0 );
}

function remote_working_type_change() {
	setDisplay('remote_wm_normal_ul', getNumber('remote_working_type') == 0 );
	setDisplay('remote_wm_xfer_ul', getNumber('remote_working_type') == 1 );
}

function remote_proto_change() {
	setDisplay('remote_proto_ms_addr', (getNumber('remote_proto_ms')== 0)&&getNumber('remote_proto')==PROTO_MODBUS_RTU_OVER_TCP);
}

function remote_xfer_gw_trt_dst_type_change() {
	var is_gw = getNumber('remote_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('remote_xfer_mode')== XFER_M_TRT;
	var dst_type = -1;
	if(is_gw||is_trt) {
		dst_type = getNumber('remote_xfer_gw_trt_dst_type');
	}
	setDisplay('remote_xfer_gw_trt_dst_gprs_index_ul', (is_gw||is_trt)&&dst_type==PROTO_DEV_GPRS);
	setDisplay('remote_xfer_gw_trt_dst_uart_type_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('remote_xfer_gw_trt_dst_uart_baudrate_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
	setDisplay('remote_xfer_gw_trt_dst_uart_bits_ul', (is_gw||is_trt)&&dst_type>=PROTO_DEV_RS1&&dst_type<=PROTO_DEV_RS_MAX);
}

function remote_xfer_mode_change() {
	var is_gw = getNumber('remote_xfer_mode')== XFER_M_GW;
	var is_trt = getNumber('remote_xfer_mode')== XFER_M_TRT;
	setDisplay('remote_wm_xfer_proto_type_ul', is_gw);
	setDisplay('remote_xfer_gw_trt_dst_type_ul', is_gw||is_trt);
	remote_xfer_gw_trt_dst_type_change();
}

function show_remote_btn(){
	setDisplay('show_remote_dialog_btn', false);
	setDisplay('remote_proto_ms_ul', true);
	setEnable('remote_working_cs', true);
	setEnable('remote_socket_type', true);
	setEnable('remote_working_type', true);
	switch(getNumber('remote_proto')) {
	case PROTO_CC_BJDC: case PROTO_HJT212: case PROTO_DM101:
		setDisplay('show_remote_dialog_btn', true);
		setEnable('remote_working_cs', false);
		setEnable('remote_socket_type', getNumber('remote_proto') == PROTO_DM101);
		setEnable('remote_working_type', false);
		setDisplay('remote_proto_ms_ul', false);
		setValue('remote_working_cs', 0);
		setValue('remote_socket_type', 0);
		setValue('remote_working_type', 0);
		break;
	}
	remote_work_cs_change();
	remote_working_type_change();
	remote_proto_change();
	remote_xfer_mode_change();
}
//通讯协议弹窗
function show_remote_dialog(){
		setFileToTextarea();
		showDialog('remote_dialog');

		$("#dialog_remote_ok_btn").off().on("click", function(){
			saveRemoteConfig();
			hideDialog('remote_dialog');
		})
	   
}

function setFileToTextarea()
{
	if(getNumber('remote_proto')==PROTO_CC_BJDC) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_HJT212) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_hjt212_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	} else if(getNumber('remote_proto')==PROTO_DM101) {
		$.ajax({
			type: "get",
			url: "/download/" + CONFIG_PATH + "/rtu_dm101_"+getValue('remote_group')+".ini",
			dataType: "html",
			data: "",
			cache: false,
			success: function(data){
				 $("#remote_config_textarea").val(data);
			}
		}); 
	}
}

function saveRemoteConfig()
{
	var _val = $("#remote_config_textarea").val();
	/*alert(_val);
	$.ajax({
		type: "post",
		url: "/ini/upload/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini",
		data:_val,
		dataType: "html",
		async: true,
		cache: false,
		success: function(data){
			 alert("保存成功，重启生效！");
		}
	}); */
	
	var xhr = createXMLHttpRequest();
	if (xhr) {
		Show("正在保存配置,请稍后...");
		xhr.onreadystatechange = function() {
			if( xhr.readyState==4 ) {
				Close();
				if( xhr.status==200 ) {
					alert("保存成功，重启生效！");
				} else {
					alert("失败,请重试");
				}
			}
		}
		if(getNumber('remote_proto')==PROTO_CC_BJDC) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_cc_bjdc_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_HJT212) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_hjt212_"+getValue('remote_group')+".ini" );
		} else if(getNumber('remote_proto')==PROTO_DM101) {
			xhr.open("POST", "/ini/upload/" + CONFIG_PATH + "/rtu_dm101_"+getValue('remote_group')+".ini" );
		}
		xhr.send(_val);
	}
}
 function ajaxUpload(options) {
		var feature = {};
		feature.fileapi = $("<input type='file'/>").get(0).files !== undefined;
		feature.formdata = window.FormData !== undefined;

		options = options || {};
		options.type = (options.type || "GET").toUpperCase();
		options.dataType = options.dataType || "json";
		var params = formatParams(options.data);

		//创建 - 非IE6 - 第一步
		if (window.XMLHttpRequest) {
			var xhr = new XMLHttpRequest();
		} else { //IE6及其以下版本浏览器
			var xhr = new ActiveXObject('Microsoft.XMLHTTP');
		}

		//接收 - 第三步
		xhr.onreadystatechange = function () {
			if (xhr.readyState == 4) {
				var status = xhr.status;
				if (status >= 200 && status < 300) {
					options.success && options.success(xhr.responseText, xhr.responseXML);
				} else {
					options.error && options.error(status);
				}
			}
		}

		//连接 和 发送 - 第二步
		if (options.type == "GET") {
			xhr.open("GET", options.url + "?" + params, true);
			xhr.send(null);
		} else if (options.type == "POST") {
			options.xhr = xhr;
			//设置表单提交时的内容类型
			if(!feature.formdata){
				fileUploadIframe(options)
			}else{
				fileUploadXhr(options);
			}
		　　
		}
	}
//格式化参数
function formatParams(data) {
	var arr = [];
	for (var name in data) {
		arr.push(encodeURIComponent(name) + "=" + encodeURIComponent(data[name]));
	}
	arr.push(("v=" + Math.random()).replace(".",""));
	return arr.join("&");
}

function fileUploadXhr(s){
	 var form = document.getElementById(s.formId);
	 var formData = new FormData(form);
	  s.url = form.action;
 
		if (s.xhr.upload) {
			s.xhr.upload.onprogress = function(event) {
				var percent = 0;
				var position = event.loaded || event.position; /*event.position is deprecated*/
				var total = event.total;
				if (event.lengthComputable) {
					percent = Math.ceil(position / total * 100);
				}
				s.uploadProgress(event, position, total, percent);
			};
		}
		s.xhr.open('POST', form.action);
		s.xhr.send(formData);
}

/**ie低版本浏览器上传文件**/
function fileUploadIframe(options){
	var form = document.getElementById(options.formId);
	form.submit();
	/* var uid = new Date().getTime(),idIO='jUploadFrame'+uid,_this=this;
	var jIO=$('<iframe name="'+idIO+'" id="'+idIO+'" style="display:block">').appendTo('body');
	$(form).attr("target",idIO);

	function onLoad(){  
			if (options.success)  
				options.success();  

			if (options.complete)  
				options.complete();  
			 
		}  

		try{
			form.submit() 
		} catch(e){
			options.error(); 
		} ;

	 var io = document.getElementById(idIO);
	io.onload = function(){ 
		onLoad();
	} 

  
	setTimeout(function()  
		{   try   
			{  
				$(io).remove();  
				$(form).removeAttr("target");	
			} catch(e)   
			{  
				console.log("异常"); 
			}									 

		}, 1000); 
*/
}

function date_check(_d) {
	var date = _d;
	var result = date.match(/^(\d{1,4})(-|\/)(\d{1,2})\2(\d{1,2})$/);

	if (result == null)
		return false;
	var d = new Date(result[1], result[3] - 1, result[4]);
	return (d.getFullYear() == result[1] && (d.getMonth() + 1) == result[3] && d.getDate() == result[4]);

}

function __to_timestamp(_d)
{
	var _t = Date.parse(new Date(_d)) / 1000;
	return parseInt(_t & 0xFFFFFFFF);
}

function format_log_lvl(lvl)
{
	switch(lvl) {
	case 0: return "异常";
	case 1: return "错误";
	case 2: return "警告";
	case 3: return "信息";
	case 4: return "调试";
	case 5: return "详细";
	}
	
	return "未知";
}

function refresh_rtu_log(loglst)
{
	myTableItemRemoveAll('rtu_log_table');
	var table = window.document.getElementById("rtu_log_table");
	for(var n = 0; n < loglst.length; n++) {
		var row = table.insertRow(table.rows.length);
		row.style.height="25px";
		var obj = row.insertCell(0);
		obj.innerHTML = loglst[n]['n'];
		obj = row.insertCell(1);
		obj.innerHTML = loglst[n]['t'];
		obj = row.insertCell(2);
		obj.innerHTML = format_log_lvl(loglst[n]['l']);
		obj = row.insertCell(3);
		obj.innerHTML = loglst[n]['g'];
	}
}

function rtu_log_delete()
{
	if (confirm("确定清空？\n\n 该操作不可恢复！")){
		MyGetJSONWithArg("正在清空设备日志,请稍后...","/cgi-bin/delLogData?", "", function (res) {
			if( res != null && 0 == res.ret ) {
				alert("清空设备日志成功！");
			} else {
				alert("清空设备日志失败,请重试");
			}
		});
	}
}

function rtu_log_download()
{
	var start_time = 0;
	var end_time = -1;
	
	var log_start_time = getValue('log_start_time');
	var log_end_time = getValue('log_end_time');
	
	if(log_start_time.length > 0 && !date_check(getValue('log_start_time'))) {
		alert("请输入正确格式的开始日期(例：2017-01-01)");
		return ;
	}
	if(log_start_time.length > 0) start_time = __to_timestamp(log_start_time);
	if(log_end_time.length > 0 && !date_check(getValue('log_end_time'))) {
		alert("请输入正确格式的结束日期(例：2017-01-02)");
		return ;
	}
	if(log_end_time.length > 0) end_time = __to_timestamp(log_end_time);
	
	var setval = {
		'start':start_time, 
		'end':end_time
	}
	
	MyGetJSONWithArg("正在查询日志,请稍后...","/cgi-bin/getLogData?", JSON.stringify(setval), function (res) {
		if( res != null && 0 == res.ret ) {
			refresh_rtu_log(res.logs);
		} else {
			alert("查询失败,请重试");
		}
	});
}
