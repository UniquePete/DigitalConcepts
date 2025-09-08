# DigitalConcepts
Digital Concepts LoRa Project<br />
For project details, see http://digitalconcepts.net.au/iot/<br />
<br />
At the moment, I've just dumped a lot of the project files here. I'll get things better
organised and documented in due course.

## Arduino
Most of the material here comprises the sketches and libraries used in the projects described
on the project website identified above. More details to follow.

The one, essentially stand-alone [JavaScript] application/script here is the syntax highlighter.
This script is based on that developed by Indrek Luuk (see https://circuitjournal.com/syntax-highlighter).
The present version is an in-line version that includes some additional Arduino IDE syntax highlighting,
basic HTML syntax highlighting and optional line numbering. It requires the additional definition of
appropriate CSS style definitions (see Highlight.css).

The script and CSS files need to be placed in appropriate directories and referenced in the Head section
of the relevant HTML file:
```
<head>
	<script type="text/javascript" src="scripts/SyntaxHighlighter.js"></script>
	<script>
		// Run any required Syntax Highlighters when the page loads
		document.addEventListener('DOMContentLoaded', processAllArduinoCode);
		document.addEventListener('DOMContentLoaded', processAllHtmlCode);
	</script>
	<link rel="stylesheet" href="styles/Highlight.css">
</head>
```
For the time being, refer to the SyntaxHighlighter page
(https://digitalconcepts.net.au/arduino/index.php?op=SyntaxHighlighter) on the project website for full
details of the capabilities and how to use the script.

## Eagle
These are the Eagle board and schematic files for the PCBs described on the propject website referenced above.
More details to follow.
 
## Fritzing Parts
These are the Fritzing part files, and the related breadboard images to assist in identification, that are
described on the project website referenced above.
More details to follow.
