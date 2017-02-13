<?

function file_list($d,$x){
  foreach(array_diff(scandir($d),array('.','..')) as $f)if(is_file($d.'/'.$f)&&(($x)?ereg($x.'$',$f):1))$l[]=$f;
  return $l;
}

function dir_list($d){
  foreach(array_diff(scandir($d),array('.','..')) as $f)if(is_dir($d.'/'.$f))$l[]=$f;
  return $l;
} 

function makePage($name, $title, $template, $variables) {

  $page = new Template('tpl/');
  $page->set('id', $name);
  foreach($variables as $k=>$v)
    $page->set($k, $v);

  $container = new Template('tpl/');
  $container->set('id', $name);
  $container->set('title', $title);
  $keywords    = null;
  $description = null;
  if (isset($pages)) {
    $keywords    = $pages[$name]['keywords'];
	$description = $pages[$name]['description'];
  }
  $container->set('meta', array('keywords'=>$keywords, 'description'=>$description));
  $container->set('content', $page->fetch($template));

  echo $container->fetch('page.tpl.php');
}


/*  applies a template to each item of an array and return an array of the resulting strings */
function applyTemplate($input, $template, $var = 'item', $variables = array()) {
  $output = array();

  $tpl = new Template('tpl/');
  foreach($variables as $k=>$v)
    $tpl->set($k, $v);
  foreach($input as $i) {
    $tpl->set($var, $i);
    $output[] = $tpl->fetch($template);
  }
  return $output;
}

?>
