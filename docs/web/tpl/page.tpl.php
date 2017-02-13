<? echo '<?xml version="1.0" encoding="UTF-8"?>'; ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <title><?=$title?></title>
  <? foreach($meta as $n=>$c) { ?>
    <meta name="<?=$n?>" content="<?=$c?>"/>
  <? } ?>

  <link rel="stylesheet" href="css/bootstrap.min.css" type="text/css" media="screen, projection">
  <link rel="stylesheet" href="css/style.css" type="text/css" media="screen, projection"/>
 
</head>
<body>
	<div id="header" class="navbar-collapse">
	    <h1>Nexus</red></h1>
		<ul>
		<li><a href="webgl">WebGL</a>
		<li><a href="documentation">Demo</a>
		<li><a href="download">Download</a>
		<li><a href="documentation">Docs</a>
		<li><a href="contacts">Contacts</a>
	</div><! -- header -->

    <?=$content?>

	<div id="footer" class="row">
		<div class="col-sm-8 col-sm-offset-2">
			<hr style="clear:both"/>
			<p>A <a href="vcg.isti.cnr.it">Visual Computing Laboratory</a> - <a href="www.isti.cnr.it">ISTI</a> - <a href="www.cnr.it">CNR</a></p>
		</div>
	</div> <!-- footer -->

	<script src="/js/jquery-1.11.2.min.js"></script>
	<script src="/js/bootstrap.min.js"></script>
</body>
</html>

