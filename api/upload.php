<?php
// mkdir and chmod $application and version folder to 777
//
$application = $_POST["application"];
$target_dir = "$application/";
$image = basename($_FILES["file"]["name"]);
$target_file = $target_dir . $image;
if (move_uploaded_file($_FILES["file"]["tmp_name"], $target_file)) {
  $last = array('version' => $_POST["version"],
                'created' => $_POST["created"],
                'protocol' => "http:",
                'host' => $_POST["host"],
                'path' => "/api/".$application."/".$image);
  $data = array('application' => $application, 'last' => $last);
  $json_text = str_replace("\/", "/", strval(json_encode($data)));
  $file = $application."/versions/image".substr($image, -5,1);
  file_put_contents($file, $json_text);
  echo "The file $image has been uploaded. \n";
} else {
  echo "Sorry, there was an error uploading your file. \n";
}
?>
