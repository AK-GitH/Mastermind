<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
  <h1>F28HS Coursework 2 "Master Mind" Pair Project</h1>

  <h2>Contents</h2>
  <p>This folder contains the following files for the coursework:</p>
  <ul>
    <li><code>master-mind.c</code>: Main C program for the implementation</li>
    <li><code>mm-matches.s</code>: Matching function implemented in ARM Assembler</li>
    <li><code>lcdBinary.c</code>: Low-level code for hardware interaction (to be implemented in inline Assembler)</li>
    <li><code>testm.c</code>: Testing function to test C vs Assembler implementations</li>
    <li><code>test.sh</code>: Script for unit testing the matching function</li>
  </ul>

  <h2>How to Play MasterMind:</h2>
    <p>MasterMind is a two-player game between a codekeeper and a codebreaker.</p>
    <p>1. The codekeeper selects a sequence of colored pegs, hiding it from the codebreaker.</p>
    <p>2. The codebreaker tries to guess the hidden sequence by composing their own sequence of colored pegs.</p>
    <p>3. After each guess, the codekeeper provides feedback:</p>
    <ul>
        <li>Number of pegs in the guess sequence that are both of the right color and at the right position.</li>
        <li>Number of pegs in the guess sequence that are of the right color but not in the right position.</li>
    </ul>
    <p>4. The codebreaker uses this feedback to refine their guess in the next round.</p>
    <p>5. The game continues until the codebreaker successfully guesses the code or runs out of turns.</p>

  <h2>Gitlab Usage</h2>
  <p>Fork and Clone the gitlab repo to get started on the coursework. Complete the functions in <code>master-mind.c</code> and <code>lcdBinary.c</code>. For the final implementation, low-level functions for controlling LED, button, and LCD display should be implemented in inline Assembler. The matching function should be implemented in ARM Assembler. Integrate both C and Assembler components into the main program.</p>

  <h2>Building and Running the Application</h2>
  <p>You can build and run the C program using the provided Makefile commands. Use <code>make all</code> to build, <code>make run</code> to run the program in debug mode, <code>make unit</code> for unit testing the matching function, and <code>make test</code> for testing C vs Assembler version of the matching function.</p>

  <h2>Unit Testing</h2>
  <p>An example of unit-testing on 2 sequences (C part only) is as follows:</p>
  <pre>./cw2 -u 121 313</pre>
  <p>The general format for the command line is:</p>
  <pre>./cw2 [-v] [-d] [-s] &lt;secret sequence&gt; [-u &lt;sequence1&gt; &lt;sequence2&gt;]</pre>

  <h2>Wiring</h2>
  <p>For the hardware setup, follow these instructions:</p>
  <ul>
    <li><strong>Green LED:</strong> Connect to GPIO pin 13</li>
    <li><strong>Red LED:</strong> Connect to GPIO pin 5</li>
    <li><strong>Button:</strong> Connect to GPIO pin 19</li>
  </ul>
  <p>Resistors and a potentiometer are required for controlling current and contrast.</p>
</body>
</html>


