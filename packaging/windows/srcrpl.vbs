Option Explicit
Dim fso,strFilename,strSearch,strReplace,objFile,oldContent,newContent

Sub ReplaceAllByExpression(ByRef StringToExtract, ByVal MatchPattern, ByVal ReplacementText)
	Dim regEx, CurrentMatch, CurrentMatches
	Set regEx = New RegExp
	regEx.Pattern = MatchPattern
	regEx.IgnoreCase = False
	regEx.Global = True
	regEx.MultiLine = True
	StringToExtract = regEx.Replace(StringToExtract, ReplacementText)
	Set regEx = Nothing
End Sub

strFilename=WScript.Arguments.Item(0)
strSearch=WScript.Arguments.Item(1)
strReplace=WScript.Arguments.Item(2)
 
'Does file exist?
Set fso=CreateObject("Scripting.FileSystemObject")
if fso.FileExists(strFilename)=false then
   wscript.echo "file not found!"
   wscript.Quit
end if
 
'Read file
set objFile=fso.OpenTextFile(strFilename,1)
oldContent=objFile.ReadAll
 
'Do the replacements
'newContent=replace(oldContent,strSearch,strReplace,1,-1,0)
newContent = oldContent
ReplaceAllByExpression newContent, strSearch, strReplace

'Write the file back
set objFile=fso.OpenTextFile(strFilename,2)
objFile.Write newContent
objFile.Close
