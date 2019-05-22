
var POST = function(api, data, func ){
	var request = new XMLHttpRequest();
	
	request.onreadystatechange = function(){
		if(request.readyState == 4){
			try {
				//console.log(request.response);
				var resp = JSON.parse(request.response);
			} catch (e){
				var resp = {
					status: 'error',
					data: 'req error: [' + request.responseText + ']'
				};
			}
			//console.log(resp.status + ': ' + resp.data);
			if (resp.status == 'error') {
                console.log( resp.status + ':' 
                    + resp.data.length +'['+ resp.data +']' );
                alert(resp.data);
				return;
			} else if (resp.status == 'login') {
                window.open("/index.php", "_self");
            }
            
			func(resp);
		}
	};
	request.open('POST', api);
	request.send(data);
	return request;
}

var _key = function (event, id) {
	if (event.which == 13 || event.keyCode == 13) {
		//code to execute here
		elem = document.getElementById(id);
		if (typeof elem.onclick == "function") {
			elem.onclick.apply(elem);
			return false;
		}
	}
	return true;
};

// round robin click;
var _clicked = function () {
	obj = arguments[0];
	for (var i = 1; i< arguments.length-1; i++) {
		if ( obj.value==arguments[i]) {
			obj.value = arguments[i+1];
			return;
		}
	}
	obj.value = arguments[1];
} 

var _getChild = function (obj, pro, value) {

	if (obj.getAttribute(pro) == value) return obj;
	var cur = obj;
	var sub=null;
	var result = null;
	do {
		sub= (sub==null)?cur.firstChild:sub.nextSibling;
		if ("INPUT:TEXTAREA:DIV".indexOf( sub.nodeName ) >= 0
			&& sub.getAttribute(pro) == value) return sub;
		if (sub.hasChildNodes()) {
			result = searchChild(sub, pro, value);
			if (result != null) break;
		}
	} while(sub != cur.lastChild);
	
	return result;
} 

var _setForm = function(target, formtype) {
	var t = document.getElementById(target);
		t.innerHTML = document.getElementById(formtype).innerHTML;
}

var _setNext = function(target, formtype) {
	// target is target object
	var fdiv = document.createElement('div');
		fdiv.setAttribute('style', 'padding:4px; margin-left:20px; background-color:#ddd;');
		fdiv.innerHTML = document.getElementById(formtype).innerHTML;
	target.parentNode.insertBefore(fdiv, target.nextSibling);
}

function numberWithCommas(x) {
    parseInt(x);
    return x.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
}

function getQueryParams() {
    qs = document.location.search.split('+').join(' ');
    var params = {},
        tokens,
        re = /[?&]?([^=]+)=([^&]*)/g;
    while (tokens = re.exec(qs)) {
        params[decodeURIComponent(tokens[1])] = decodeURIComponent(tokens[2]);
    }
    return params;
}

function setCookie(cname, cvalue, exdays) {
    var d = new Date();
    d.setTime(d.getTime() + (exdays*24*3600*1000));
    var expires = "expires="+ d.toUTCString();
    document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

function getCookie(cname) {
    var name = cname + "=";
    var decodedCookie = decodeURIComponent(document.cookie);
    var ca = decodedCookie.split(';');
    for(var i = 0; i <ca.length; i++) {
        var c = ca[i];
        while (c.charAt(0) == ' ') {
            c = c.substring(1);
        }
        if (c.indexOf(name) == 0) {
            return c.substring(name.length, c.length);
        }
    }
    return "";
} 

var CURR_PAGE = 0;
var LAST_PAGE = 0;
var set_pagination = function(service, current, total) {
    spansize = 10
    halfsize = 5
    console.log('service:'+service+', current"'+current+', total:'+total+'.');
    LAST_PAGE= total;
    begin_page = last_page = cur_page = batches = 0;
    
    begin_page =  (current < halfsize ) ? 0: current - halfsize;
    last_page  =  (begin_page + spansize > total ) ? total : begin_page + spansize;
    console.log('begin_page:'+begin_page+', current"'+current+', last_page:'+last_page+'.');
    
    pagehtml = '';
    for (p = begin_page; p <= last_page; p++) {
        if (p == current ) {
            pagehtml += "<span class='current' >["+(p+1)+"]</span>\n"
        } else {
            pagehtml += "<span onclick='"+service+"("+p+");'>&nbsp;"+(p+1)+"&nbsp;</span>\n"
        }
    }
    document.getElementById('paging').innerHTML = pagehtml;
}
