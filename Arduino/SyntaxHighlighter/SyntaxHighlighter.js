/*
		Code Syntax Highlighter
		
		This script is based on that developed by Indrek Luuk
		(see https://circuitjournal.com/syntax-highlighter). The present version is an in-line
		version that includes some additional syntax and optional line numbering. It requires
		the additional definition of appropriate CSS style definitions (see Highlight.css).

    250531	1.0.0		Initial release
    250601	2.0.0		Add [basic] HTML syntax highlighting
    250606	2.1.0		Rename functions for consistency between and within highlighting
    									profiles
    								Remove unused functions, constants and variables
    250612	2.2.0		Modify function highlightHtmlCode to process multiple
    									<code language="language-HTML"> elements within a <pre> element
    250706	2.2.1		Modify Arduino numeric string match to exclude numerics preceded by "_"
    250812	2.2.2		Modify Arduino numeric srting match to include qualifiers - preceding
    									"-", decimal point in floating point numbers and suffix qualifiers
    									(e.g. "f" for floating point numbers)
    250826	2.2.3		Add #if, #else & #elif to the ide-system-keyword list
    250901	2.2.4		Correct handling of block comments when included inline
    250908	2.2.5		Correct error in handling of hex numbers introduced at 2.2.2
    								Correct error in handling HTML tag attributes without values
		
    Digital Concepts
    08 Sep 2025
    digitalconcepts.net.au
*/

// Function to add Arduino IDE syntax highlight styling

function highlightArduinoCode(preElement) {
		// Get the code element
		const code = preElement.querySelector('code.language-Arduino');
		
		if (code) {
		
				const lineNumbers = code.hasAttribute('data-line-numbers');
				
				// Get the current content
				const originalContent = code.textContent;
				
				// Add syntax highlighting
				const modifiedContent = formatArduinoCode(originalContent,lineNumbers);
				
				// Create a temporary div to parse the HTML
				const tempDiv = document.createElement('div');
				tempDiv.innerHTML = modifiedContent;
				
				// Update the code content with the parsed HTML
				code.innerHTML = tempDiv.innerHTML;                
		}
}

// Function to add [BBEdit] HTML syntax highlight styling

function highlightHtmlCode(preElement) {

    // Get all matching code elements
    
    // For HTML code highlighting, there could be sections within a block that
    // are not HTML and thus do not want to be highlighted. Specifically, this
    // applies to code content, being presented within an HTML block, that includes
    // something like the "//" comment prefix. If the <code class="language-HTML"> block
    // is not terminated before the inclusion of code (which should then be included
    // within its own <code></code> block) that should be presented without any
    // highlighting, and a new <code class="language-HTML"> block started afterwards, all
    // within the one <pre> block, this content would be interpreted as an HTML comment
    // and highlighted accordingly.
    
    const codes = preElement.querySelectorAll('code.language-HTML');

    // Loop through each found code element and apply the highlighting
    codes.forEach(code => {
        const lineNumbers = code.hasAttribute('data-line-numbers');

        // Get the current content
        const originalContent = code.textContent;

        // Add syntax highlighting
        const modifiedContent = formatHtmlCode(originalContent, lineNumbers);

        // Create a temporary div to parse the HTML
        const tempDiv = document.createElement('div');
        tempDiv.innerHTML = modifiedContent;

        // Update the code content with the parsed HTML
        code.innerHTML = tempDiv.innerHTML;
    });
}

// Function to process all pre elements with Arduino code

function processAllArduinoCode() {
		// Get all pre elements containing Arduino code
		const preElements = document.querySelectorAll('pre:has(code.language-Arduino)');
		
		// Process each pre element
		preElements.forEach(pre => {
				highlightArduinoCode(pre);
		});
}

// Function to process all pre elements with HTML code

function processAllHtmlCode() {
		// Get all pre elements containing HTML code
		const preElements = document.querySelectorAll('pre:has(code.language-HTML)');
		
		// Process each pre element
		preElements.forEach(pre => {
				highlightHtmlCode(pre);
		});
}

const arduinoKeywordStyles = [
		{
				keywords: ['class', 'struct', 'void', 'static', 'const', 'inline', 'public', 'protected', 'private',
						'bool', 'boolean', 'int', 'char', 'unsigned', 'long', 'double', 'float', 'byte', 'enum', 'RadioEvents_t',
						'TimerEvent_t', 'neoPixelColour_t',
						'uint8_t', 'uint16_t', 'uint32_t', 'int8_t', 'int16_t', 'int32_t', 'sizeof'],
				styleClass: 'ide-keyword'
		},
		{
				keywords: ['#include', '#define', '#if', '#ifdef', '#ifndef', '#elif', '#else', '#endif', 'return', 'if', 'else', 'for', 'while', 'do',
						'switch', 'case', 'default', 'break'],
				styleClass: 'ide-system-keyword'
		},
		{
				keywords: ['Serial', 'SoftwareSerial', 'ArduinoOTA', 'WiFi', 'EEPROM', 'ESP8266WebServer', 'IPAddress',
						'ESP'],
				styleClass: 'ide-platform-class'
		},
		{
				keywords: ['setup', 'loop', 'begin', 'print', 'println', 'printf', 'pinMode', 'digitalRead', 'digitalWrite', 'analogRead', 'analogWrite',
						'delay', 'millis', 'blink','ESP8266WiFi', 'waitForConnectResult', 'localIP', 'mode', 'handle',
						'handleClient', 'on', 'hasArg', 'arg', 'read', 'write', 'softAPConfig', 'softAP', 'softAPIP', 'toCharArray', 'send',
						'restart', 'sizeof', 'available', 'readBytes'],
				styleClass: 'ide-platform-function'
		},
		{
				keywords: ['true', 'false'],
				styleClass: 'ide-literal'
		},
];

const blockCommentStyleClass = 'ide-block-comment';
const lineCommentStyleClass = 'ide-line-comment';
const stringStyleClass = 'ide-string';
const characterStyleClass = 'ide-keyword';
const numericStyleClass = 'ide-numeric';
const platformFunctionStyleClass = 'ide-platform-function';

const htmlBlockCommentStyleClass = 'html-block-comment';
const htmlLineCommentStyleClass = 'html-line-comment';
const htmlStringStyleClass = 'html-string';
const htmlTagStyleClass = 'html-tag';

const lineNumberTablePrefix = '<table class="highlight-lines"><tbody>';
const lineNumberRowPrefix = '<tr><td class="highlight-line-num">';
const lineNumberRowInfix = '</td><td class="highlight-line-code">';
const lineNumberRowSuffix = '</td></tr>';
const lineNumberTableSuffix = '</tbody></table>';

function formatArduinoCode(code,includeLineNumbers) {
//    		let includeLineNumbers = code.getAttribute("data-line-numbers");
//    		let includeLineNumbers = false;
		let lineNumber = 0;
		let codeLines = code.split("\n");
//        codeLines.push('');

		let blockComment = false;
		let inlineBlockComment = false;
		let endBlockComment = false;
		let lineStartsInBlockComment = false;
		for (let i=0; i<codeLines.length; i++) {
				lineNumber = i + 1;
				// Add a dummy block comment start for block comment lines,
				// so that segment splitting can work properly
				let codeLine = lineStartsInBlockComment ? '/*' + codeLines[i] : codeLines[i];

				// Split comments and strings from the keyword highlighted code.
				let codeSegments = splitLineSegments(codeLine, [
						{start: '/*', end: '*/'},
						{start: '//', end: null},
						{start: '"', end: '"'},
						{start: "'", end: "'"}]);

				let highlightedLine = "";
				for (let j=0; j<codeSegments.length; j++) {
						let codeSegment = codeSegments[j];

						// Handle block comments
						if (codeSegment.startsWith('/*')) {
								if ( j == 0 ) {
										blockComment = true;
								} else {
										inlineBlockComment = true;
								}
								if (!lineStartsInBlockComment) {
										// Beginning of new block comment
										if (includeLineNumbers && blockComment) {
											highlightedLine += lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + '<span class="'+blockCommentStyleClass+'">' + escapeHtml(codeSegment) + '</span>' + lineNumberRowSuffix;
										} else {
											highlightedLine += '<span class="'+blockCommentStyleClass+'">' + escapeHtml(codeSegment);
										}
								} else {
										// If block comment line then remove the dummy block comment start
										if (includeLineNumbers) {
											highlightedLine += lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + '<span class="'+blockCommentStyleClass+'">' + escapeHtml(codeSegment.substring(2)) + '</span>' + lineNumberRowSuffix;
										} else {
											highlightedLine += escapeHtml(codeSegment.substring(2));
										}
								}
								if (codeSegment.endsWith('*/')) {
										// End of block comment
										if (!includeLineNumbers || inlineBlockComment) {
											highlightedLine += '</span>'
										}
										lineStartsInBlockComment = false;
										inlineBlockComment = false;
										endBlockComment = true;
								} else {
										// This line didn't end the current block comment
										lineStartsInBlockComment = true;
								}

								// Handle line comment
						} else if (codeSegment.startsWith('//')) {
								highlightedLine += '<span class="'+lineCommentStyleClass+'">' + escapeHtml(codeSegment) + '</span>';

								// Handle strings
						} else if (codeSegment.startsWith('"')) {
								if (codeSegment.endsWith('"')) {
										highlightedLine += '<span class="'+stringStyleClass+'">' + escapeHtml(codeSegment) + '</span>';
								} else {
										// Do not highlight as string if it wasn't properly closed
										highlightedLine += escapeHtml(codeSegment);
								}

								// Handle single characters
						} else if (codeSegment.startsWith("'")) {
								if (codeSegment.length === 3 && codeSegment.endsWith("'")) {
										highlightedLine += '<span class="'+characterStyleClass+'">' + escapeHtml(codeSegment) + '</span>';
								} else {
										// Do not highlight as character if it wasn't a single character or wasn't properly closed
										highlightedLine += escapeHtml(codeSegment);
								}

								// Handle highlighted keywords
						} else {
								highlightedLine += formatArduinoKeywords(escapeHtml(codeSegment));
						}
				}

				if (includeLineNumbers) {
					if ( blockComment ) {
						codeLines[i] = highlightedLine;
						if ( endBlockComment ) {
							blockComment = false;
							endBlockComment = false;
						}
					} else {
						codeLines[i] = lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + highlightedLine + lineNumberRowSuffix;
					}
				} else {
					codeLines[i] = highlightedLine;
				}
		}
		let formattedCode = '';
		if (includeLineNumbers) {
			formattedCode = lineNumberTablePrefix + codeLines.join('\n') + lineNumberTableSuffix;
		} else {
			formattedCode = codeLines.join('\n');
		}
		return formattedCode;
}

function formatHtmlTag(html) {
    // Pattern to match tag, attributes, and content
    const pattern = /<([^\s>]+)\s*([^>]*?)>|<\/([^\s>]+)>/g;
    let result = '';
    let lastIndex = 0;
    
    // Find all matches
    while (true) {
        const match = pattern.exec(html);
        if (!match) break;
        
        // Add text content before the tag
        if (match.index > lastIndex) {
            result += escapeHtml(html.substring(lastIndex, match.index));
        }
        
        // Get the tag content
        const tagContent = match[0];
        
        // Check if it's a closing tag
        if (tagContent.startsWith('</')) {
        	if (tagContent.substring(2,4) == "a>") {
            result += `<span class="html-hyperlink">${escapeHtml(tagContent)}</span>`;
        	} else {
            result += `<span class="html-tag">${escapeHtml(tagContent)}</span>`;
        	}
        } else {
            // Handle opening tag with attributes
            const tag = match[1];
            const attributes = match[2];
            
            if (tagContent.substring(1,2) =="a") {
							result += `<span class="html-hyperlink">&lt;${escapeHtml(tag)}</span>`;
            } else if (tagContent.substring(1,4) =="img") {
							result += `<span class="html-image">&lt;${escapeHtml(tag)}</span>`;
            } else {
							// Add opening tag
							result += `<span class="html-tag">&lt;${escapeHtml(tag)}</span>`;
						}
							
						// Split attributes using regex
						const attrPattern = /([^\s=]+)(?:="([^"]*)")?/g;
						let attrMatch;
						
						while ((attrMatch = attrPattern.exec(attributes)) !== null) {
								// Skip empty matches
								if (!attrMatch[1]) continue;
								
								// Add attribute
								result += ` <span class="html-attribute">${escapeHtml(attrMatch[1])}</span>`;
								
								// Add value if it exists
								if (attrMatch[2] !== undefined) {
										result += `=<span class="html-attribute-value">"${escapeHtml(attrMatch[2])}"</span>`;
								}
						}							
							
						// Add closing >
            if (tagContent.substring(1,3) =="a ") {
							result += `<span class="html-hyperlink">` + escapeHtml(">") + `</span>`;
            } else if (tagContent.substring(1,5) =="img ") {
							result += `<span class="html-image">` + escapeHtml(">") + `</span>`;
						} else {
							result += `<span class="html-tag">` + escapeHtml(">") + `</span>`;
            }
        }
        
        lastIndex = pattern.lastIndex;
    }
    
    // Add remaining text content
    if (lastIndex < html.length) {
        result += escapeHtml(html.substring(lastIndex));
    }
    
    return result;
}

function formatHtmlCode(code,includeLineNumbers) {
//    		let includeLineNumbers = code.getAttribute("data-line-numbers");
//    		let includeLineNumbers = false;
		let lineNumber = 0;
		let codeLines = code.split("\n");
//        codeLines.push('');

		let blockComment = false;
		let endBlockComment = false;
		let lineStartsInBlockComment = false;
		for (let i=0; i<codeLines.length; i++) {
				lineNumber = i + 1;
				// Add a dummy block comment start for block comment lines,
				// so that segment splitting can work properly
				let codeLine = lineStartsInBlockComment ? '/*' + codeLines[i] : codeLines[i];

				// Split comments from HTML tags
				let codeSegments = splitLineSegments(codeLine, [
						{start: '/*', end: '*/'},
						{start: '//', end: null},
						{start: '<', end: '>'}]);

				let highlightedLine = "";
				for (let j=0; j<codeSegments.length; j++) {
						let codeSegment = codeSegments[j];

						// Handle block comments
						if (codeSegment.startsWith('/*')) {
								blockComment = true;
								if (!lineStartsInBlockComment) {
										// Beginning of new block comment
										if (includeLineNumbers) {
											highlightedLine += lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + '<span class="'+htmlBlockCommentStyleClass+'">' + escapeHtml(codeSegment) + '</span>' + lineNumberRowSuffix;
										} else {
											highlightedLine += '<span class="'+htmlBlockCommentStyleClass+'">' + escapeHtml(codeSegment);
										}
								} else {
										// If block comment line then remove the dummy block comment start
										if (includeLineNumbers) {
											highlightedLine += lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + '<span class="'+htmlBlockCommentStyleClass+'">' + escapeHtml(codeSegment.substring(2)) + '</span>' + lineNumberRowSuffix;
										} else {
											highlightedLine += escapeHtml(codeSegment.substring(2));
										}
								}
								if (codeSegment.endsWith('*/')) {
										// End of block comment
										if (!includeLineNumbers) {
											highlightedLine += '</span>'
										}
										lineStartsInBlockComment = false;
										endBlockComment = true;
								} else {
										// This line didn't end the current block comment
										lineStartsInBlockComment = true;
								}

								// Handle line comment
						} else if (codeSegment.startsWith('//')) {
								highlightedLine += '<span class="'+htmlLineCommentStyleClass+'">' + escapeHtml(codeSegment) + '</span>';

								// Handle HTML tags
						} else if (codeSegment.startsWith("<")) {
								if (codeSegment.endsWith(">")) {
//										highlightedLine += '<span class="'+htmlTagStyleClass+'">' + formatHtmlKeywords(escapeHtml(codeSegment)) + '</span>';
                    highlightedLine += formatHtmlTag(codeSegment);

								} else {
										// Do not highlight as an HTML tag if part of a comment
										highlightedLine += escapeHtml(codeSegment);
								}

								// No action required
						} else {
								highlightedLine += escapeHtml(codeSegment);
						}
				}

				if (includeLineNumbers) {
					if ( blockComment ) {
						codeLines[i] = highlightedLine;
						if ( endBlockComment ) {
							blockComment = false;
							endBlockComment = false;
						}
					} else {
						codeLines[i] = lineNumberRowPrefix + lineNumber.toString() + lineNumberRowInfix + highlightedLine + lineNumberRowSuffix;
					}
				} else {
					codeLines[i] = highlightedLine;
				}
		}
		let formattedCode = '';
		if (includeLineNumbers) {
			formattedCode = lineNumberTablePrefix + codeLines.join('\n') + lineNumberTableSuffix;
		} else {
			formattedCode = codeLines.join('\n');
		}
		return formattedCode;
}

function splitLineSegments(codeLine, separators) {
		let codeSegments = [];
		while (codeLine.length > 0) {
				let segmentEndIndex = getEndOfCurrentSegment(codeLine, separators);
				codeSegments.push(codeLine.substring(0, segmentEndIndex));
				codeLine = codeLine.substring(segmentEndIndex, codeLine.length);
		}
		return codeSegments;
}

function getEndOfCurrentSegment(codeLine, separators) {
		let currentSeparator = getCurrentSeparator(codeLine, separators);
		if (currentSeparator != null) {
				return getSegmentEndForSeparator(codeLine, currentSeparator);
		} else {
				return getStartOfNextSegment(codeLine, separators);
		}
}

function getCurrentSeparator(codeLine, separators) {
		for (let i = 0; i < separators.length; i++) {
				if (codeLine.startsWith(separators[i].start)) {
						return separators[i];
				}
		}
		return null;
}

function getSegmentEndForSeparator(codeLine, separator) {
		if (separator.end === null) {
				return codeLine.length;
		} else {
				let currentSegmentEndIndex = codeLine.indexOf(separator.end, separator.start.length);
				return currentSegmentEndIndex >= 0 ? currentSegmentEndIndex + separator.end.length : codeLine.length;
		}
}

function getStartOfNextSegment(codeLine, separators) {
		let nextSegmentStartIndex = codeLine.length;
		for (let i=0; i < separators.length; i++) {
				let segmentStartIndex = codeLine.indexOf(separators[i].start);
				if (segmentStartIndex >= 0 && segmentStartIndex < nextSegmentStartIndex) {
						nextSegmentStartIndex = segmentStartIndex;
				}
		}
		return nextSegmentStartIndex;
}

function formatArduinoKeywords(codeLine) {
		function addHighlight(keyword, className) {
				let keywordRegEx = keyword.indexOf("#") === 0 ?
						'('+keyword+')\\b' : '\\b('+keyword+')\\b';
				codeLine = codeLine.replace(new RegExp(keywordRegEx, 'g'), '<span class="'+className+'">$1</span>');

		}

		for (let i=0; i<arduinoKeywordStyles.length; i++) {
				for (let j=0; j<arduinoKeywordStyles[i].keywords.length; j++) {
						addHighlight(arduinoKeywordStyles[i].keywords[j], arduinoKeywordStyles[i].styleClass);
				}
		}
		// Highlight numeric strings, including HEX
		codeLine = codeLine.replace(
				new RegExp(/(?<![_a-zA-Z0-9])(-?0x[0-9a-fA-F]+)(?![a-zA-Z0-9])|(?<![_a-zA-Z0-9])(-?(?:\d+\.?\d*|\.\d+))(?:([fFlL])(?![a-zA-Z0-9])|(?![a-zA-Z0-9]))/, 'g'),
				function(match, hexMatch, decimalMatch, suffix) {
						if (hexMatch) {
								return '<span class="' + numericStyleClass + '">' + hexMatch + '</span>';
						} else {
								return '<span class="' + numericStyleClass + '">' + decimalMatch + '</span>' + (suffix || '');
						}
				}
		);
		// Highlight class instances and their methods – the function avoids catching 'local' libraries that have had their "<" character escaped
		codeLine = codeLine.replace(/(?<=\.)[a-zA-Z_$][a-zA-Z0-9_$]*|([a-zA-Z_$][a-zA-Z0-9_$]*)(?=[\.\(])/g,
				function(match, p1, offset, string) {
						// Check if this is part of a header file include
						const isHeaderFile = string.includes('<' + (p1 || match) + '.h>') || string.includes('&lt;' + (p1 || match) + '.h&gt;') || match == 'h';
						return isHeaderFile ? match : '<span class="'+platformFunctionStyleClass+'">' + (p1 || match) + '</span>';
				}
		);
		// Highlight local library header filenames, those enclosed in "<>" (just check for trailing "&gt;" at this point)
		codeLine = codeLine.replace(new RegExp(/.*?\.h&gt;[\t \n]*$/, 'gm'), '<span class="'+stringStyleClass+'">$&</span>');
		return codeLine;
}

function escapeHtml(unsafe) {
		return unsafe
				.replace(/&/g, "&amp;")
				.replace(/</g, "&lt;")
				.replace(/>/g, "&gt;")
				.replace(/"/g, "&quot;")
				.replace(/'/g, "&#039;");
}