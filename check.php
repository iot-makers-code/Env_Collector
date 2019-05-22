<?php	
$id = $_REQUEST['id'];
$t = $_REQUEST['t'];  //temperature
$h = $_REQUEST['h'];  //humidity

//loging
if (!file_exists("log")) mkdir ("log");
$logf =  date("Ymd").".log";
$logf =  'log/'.$logf;
$cotents = time()."\t".$t."\t".$h."\t".$id.:"\n";
file_put_contents($logf, $cotents, FILE_APPEND);
exit();
?>
