#!/usr/bin/php
<?php
// Call from make file as make register
//	-$(FOTA_CLIENT) --application $(PROJECT) --version $(VERSION) --i1 $(IMAGE1) --i2 $(IMAGE2) --config ${FOTA_CONFIGFILE} --profile ${FOTA_PROFILE}
//	-$(FOTA_CLIENT) --application mqttota --version 0.0.2 --i1 .firmwares/4096_user1.bin --i2 .firmwares/4096_user2.bin --config /home/arendst/.fotaclient-config.json --profile DEMO or LOCAL
//	-$(FOTA_CLIENT) --application mqttota --version 0.0.2 --i1 .firmwares/user1.bin --i2 .firmwares/user2.bin
// var_dump($argv);

//$host = "deb8gui";
$host = "sidnas2";

$application = $argv[2]; // "mqttota"
$version = $argv[4];     // "0.0.2"
$images[0] = $argv[6];   // ".firmwares/4096_user1.bin"
$images[1] = $argv[8];   // ".firmwares/4096_user2.bin"
$url = 'http://'.$host.'/api/upload.php';

foreach ($images as $image) {
  if (file_exists($image)) {
    $created = date("Y-m-d\TH:i:s", filemtime($image)).".0000";
    $request = curl_init($url);
    $cfile = new CurlFile($image, 'image/png');
    $args = array('file' => $cfile, 'application' => $application, 'version' => $version, 'created' => $created, 'host' => $host);
    curl_setopt($request, CURLOPT_POST, true);
    curl_setopt($request, CURLOPT_POSTFIELDS, $args);
    curl_setopt($request, CURLOPT_RETURNTRANSFER, true);
    echo curl_exec($request);
    curl_close($request);
  }
}

?>
