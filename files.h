#pragma once

const String index_html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>WIFI-GEN</title>
</head>
<body>
    <center>
        <h1>Sinus DDS Generator 0-50MHz</h1>
    </center>
    <center>
        <h2>1 Hz Resolution</h2>
    </center>
    <form action="/set" method="get">
        Input Frequency:<br>
        <input max="50000000" min="1" name="freq" type="number"> <input type="submit" value="Submit">
    </form>
</body>
</html>
)=====";