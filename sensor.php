<?php 
$trace=true;

function s00_log($msg) {
    global $trace;
    if ($trace) error_log($msg);
}

function outputJSON($msg, $status = 'error'){

    if ($status == 'error') error_log (print_r($msg, true)." in ".__FILE__); 
    
    header('Content-Type: application/json');
    die(json_encode(array(
        'data' => $msg,
        'status' => $status
    )));
}

/////////////////////////////////////
// null service
$services['test'] = '_test';
function _test() { 
    s00_log ("Start ".__FUNCTION__);
    throw new Exception ( __FILE__.'is Available.');
};

//////////////////////////////////////////////////
// get receive info
$services['newnode'] = '_newnode';
function _newnode() { 
    s00_log ("Start ".__FUNCTION__);
	
	$mode = $_REQUEST['mode'];
	$mac = $_REQUEST['mac'];
	$ip = $_REQUEST['ip'];

	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	
	$nodes = json_decode(file_get_contents($nodesf), true);
	//id define
	$id = $ip."-".$mac;
	if (!isset($nodes[$id])) {
		$nodes[$id] = array( 'ip'=>$ip, 'mac'=>$mac, 'mode'=>$mode, 'time' => time() );
	} else {
		$nodes[$id]['mode']=$mode;
		$nodes[$id]['time']=time();
	}
	
	file_put_contents($nodesf, json_encode($nodes));
	outputJSON("$ip registed", 'success');
};

/////////////////////////////////////
// undate node status
$services['set_node'] = '_set_node';
function _set_node() { 
    s00_log ("Start ".__FUNCTION__);

    if (!isset($_POST["table"])) 
        throw new Exception ("자료[table]이 필요합니다.");

    $nodes_n = array_values(json_decode($_POST["table"], true));
	//error_log(print_r($nodes_n, true));
	
	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	$nodes = json_decode(file_get_contents($nodesf), true);
	//id define
	foreach ($nodes_n as $node) {
		error_log(print_r($node, true));
		$id = $node['id'];
		if (isset($nodes[$id])) {
			error_log("ID :::::::::::: $id");
			if (isset($nodes[$id])) $nodes[$id]['loc'] = $node['loc'];
			//error_log("name:".$nodes[$id]['loc']);
		} else {
			$nodes[$id] = array( 'ip'=>$ip, 'mac'=>$mac, 
						'mode'=>$mode, 'time' => time(),
						'loc' =>$node['loc']);			
		}
	}
	
	file_put_contents($nodesf, json_encode($nodes));	
    
    outputJSON("Save to ..", 'success');
};

/////////////////////////////////////
// initial node
$services['init_node'] = '_init_node';
function _init_node() { 
    s00_log ("Start ".__FUNCTION__);

	$id = $_POST['id'];

	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	$nodes = json_decode(file_get_contents($nodesf), true);
	//id define
	$nodes[$id]['status'] = 'new';
	file_put_contents($nodesf, json_encode($nodes));	
	
	//loging
	$logf =  'node/log/'.date("Ymd-").$id.".log";
	if (!file_exists("node/log")) mkdir ("node/log");
	
	//unlink ($logf);
    
    outputJSON("Save to ..", 'success');
};


//////////////////////////////////////////////////
// set received sensor data
$services['check'] = '_check';
function _check() { 
    s00_log ("Start ".__FUNCTION__);
	
	$ip = $_REQUEST['ip'];
	$c = $_REQUEST['c'];  //count
	$t = $_REQUEST['t'];  //temperature
	$h = $_REQUEST['h'];  //humidity
	$d = isset($_REQUEST['d'])?$_REQUEST['d']:0;  // dust ppmv 
	$r = isset($_REQUEST['r'])?$_REQUEST['r']:0;  // ratio of dust sensing
	$v = isset($_REQUEST['v'])?$_REQUEST['v']:'5.0';
	$mac = $_REQUEST['mac'];

	$chkf = "checks.json";
	$checks = json_decode(file_get_contents($chkf), true);
	//id define
	$id = $ip."-".$mac;
	
	if (!isset($checks[$id])) {
		$checks[$id] = array( 'id'=>$id, 'ip'=>$ip, 'mac'=>$mac, 
			'c'=>$c, 't' => $t, 'h' => $h, 'v'=>$v, 'd' => $d, 'r'=>$r, 'time' => time() );
	} else {
		$checks[$id]['id'] = $id;
		$checks[$id]['ip'] = $ip;
		$checks[$id]['c'] =  $c;
		$checks[$id]['t'] =  $t;
		$checks[$id]['h'] =  $h;
		$checks[$id]['d'] =  $d;
		$checks[$id]['r'] =  $r;
		$checks[$id]['mac'] = $mac;
		$checks[$id]['time'] = time();
	}
	
	foreach ($checks as $k => $check) {
		if (time() - $check['time'] > 3600) {
			unset($checks[$k]);
		} else 
		if (time() - $check['time'] > 1800) {
			//error_log($k);
			$checks[$k]['c'] = 0;
			$checks[$k]['t'] = 0;
			$checks[$k]['h'] = 0;
			$checks[$k]['d'] = 0;
			$checks[$k]['r'] = 0;
		}
	}
	file_put_contents($chkf, json_encode($checks));
	
	//loging
	$logf =  date("Ymd-").$id.".log";
	if (!file_exists("node/log")) mkdir ("node/log");
	$logf =  'node/log/'.$logf;
	$cnt = time()."\t".$t."\t".$h."\t".$c."\t".$r."\t".$d."\n".file_get_contents($logf);
	file_put_contents($logf, $cnt);
	
	//node control with return message
	$result = "";
	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	$nodes = json_decode(file_get_contents($nodesf), true);
	if ( isset($nodes[$id]['status']) 
		&& $nodes[$id]['status'] == 'new') $result = "init";
	$nodes[$id]['status'] = '';
	file_put_contents($nodesf, json_encode($nodes));	
	
	$result = "init;10";	
	outputJSON($result, 'success');
};

//////////////////////////////////////////////////
// set received sensor data
$services['status'] = '_status';
function _status() { 
    s00_log ("Start ".__FUNCTION__);
	
	$chkf = "checks.json";
	$checks = json_decode(file_get_contents($chkf), true);

	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	$nodes = json_decode(file_get_contents($nodesf), true);
	
	foreach ($checks as $k => $check) {
		if (time() - $check['time'] > 1800) {
			unset($checks[$k]);
		} else {
			$checks[$k]['time'] = time() - $checks[$k]['time'];
			$checks[$k]['loc'] = isset($nodes[$k])? $nodes[$k]['loc'] : '';
		}
	}
	
    $checks = array_values($checks);
	outputJSON($checks, 'success');
};


/////////////////////////////////////
// details 
$services['getlog'] = '_getlog';
function _getlog() { 
    s00_log ("Start ".__FUNCTION__);

	$ipmac = $_POST['query'];
	$mode = $_POST['mode'];
	$current = $_POST['page'];
	
	//loging
	$logf =  date("Ymd-").$ipmac.".log";
	if (!file_exists("node/log")) mkdir ("node/log");
	$logf =  'node/log/'.$logf;
	if (!file_exists($logf)) throw new Exception ("File not exist");
	$fd = fopen($logf, "r");
	if ($fd) $fd = $fd;
	else throw new Exception ("File open error");
	
	$arr = array();
	$total = $count = 0;
	while(($line = fgets($fd)) !== false) {
		$count++;
		if ($count < $current*30+1 ) { continue;}
		if (($current+1)*30 < $count ) { continue;}

		$line = trim(str_replace("\n", "", $line));
		list($dt, $t, $h, $c) = explode("\t", $line);
		$x = "";
		$x['time'] = date("H:i:s", $dt);
		if (strpos($mode, "T")!==false) $x["Temperature"]=$t;
		if (strpos($mode, "H")!==false) $x["Humidity"]=$h; 
		if (strpos($mode, "C")!==false) $x["Check"]=$c;
		if (strpos($mode, "A")!==false) { 
			$x["Temperature"]=$t;
			$x["Humidity"]=$h;
			$x["Check"]=$c;
		}
		//$x = array('time'=>date("H:i:s", $dt), "Temperature"=>$t, "Humidity"=>h);
		//error_log(print_r($x, true));
		$arr[] = $x;
	}
	fclose($fd);
	$total = $count/30; 
	
	$rt['rows'] = $arr;
	//error_log(print_r($arr, true));
	//error_log("Mode:-------------".$mode);

	$nodesf = "node/state/nodes.json";
	if (!file_exists("node/state")) mkdir ("node/state");
	$nodes = json_decode(file_get_contents($nodesf), true);
	$rt['title'] = (isset($nodes[$ipmac]))?$nodes[$ipmac]['loc']:$ipmac;;
	$rt['graph'] = 'Y';
	$rt['page'] = array("current"=> $current, "total"=>$total);
    
    outputJSON($rt, 'success');
};


/////////////////////////////////////
// execute services
$func= isset($_REQUEST['func'])?$_REQUEST["func"]:"test";

if (!isset($services[$func])) 
        outputJSON("Undefined service[$func].");
try {
    call_user_func( $services[$func]);
} catch (Exception $e) {
    outputJSON($e->getMessage());
}


?>
