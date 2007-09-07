
<?
function createLink($name, $desc) {
	global $extension;
	if( strlen($extension) > 0 ) {
		echo '<a href="' . $name . $extension . '">' . $desc . "</a>\n";
	}
	else {
		echo '<a href="' . $name . '.php">' . $desc . "</a>\n";
	}
}
?>

