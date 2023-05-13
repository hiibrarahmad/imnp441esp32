const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Example</title>
</head>
<body>
<table>
<tr>
<td>
<button onClick="play('/recording.wav')" id="play1">PLAY 1</button>
</td>
</tr>
</table>
<iframe id="ifr" style="display:none"></iframe>
</body>
</html>
<script type="text/javascript">
if (window.HTMLAudioElement)
{
  var player = new Audio('');
  function play(url)
  {
    player.src = url;
    player.play();
  }
}
</script >
)rawliteral";
