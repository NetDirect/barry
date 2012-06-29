<?

// Creates a page header & linear navigation tracing back to the main page
function createHeader($title) {
	global $doc_mode;

	if( !strcmp($doc_mode, "netdirect") ) {
		echo "Paste into drupal: " . $title . "\n\n";
	}
	else {
		echo "<div class='pageHeader'>$title</div>";
		echo "<img src='nothing.jpg' width='100%' height='5' alt=''><br>";
		echo "<div class='linearNav'>";
	}
}

function createSubHeader($title) {
	global $doc_mode;

	if( !strcmp($doc_mode, "netdirect") ) {
		echo "<h1>$title</h1>";
	}
	else {
		echo '<div class="subHeader">' . $title . '</div>';
	}
}

function createLink($name, $desc) {
	global $extension;
	global $barry_page_path;

	echo '<a href="' . $barry_page_path . $name . $extension . '">' . $desc . "</a>\n";
}

function createHref($name) {
	global $extension;
	global $barry_page_path;

	echo 'href="' . $barry_page_path . $name . $extension . '"';
}

function createFileLink($name, $desc) {
	global $files_path;

	echo '<a href="' . $files_path . $name . '">' . $desc . "</a>\n";
}

function createPureFileLink($name) {
	global $doc_mode;
	global $files_path;

	if( !strcmp($doc_mode, "netdirect") ) {
		createFileLink($name, basename($name));
	}
	else {
		echo '<a href="http://www.netdirect.ca/sites/www.netdirect.ca/files/barry/' . $name . '">' . basename($name) . "</a>\n";
	}
}

function createImage($filename) {
	global $image_path;

	echo '<img src="' . $image_path . $filename . '"/>';
}

?>
