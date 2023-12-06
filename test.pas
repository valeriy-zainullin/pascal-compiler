program Hello;
var lib_token, printf_token : Integer;
begin
	lib_token := LibraryLoad("glibc.so");
  printf_token := LibraryFind(lib_token, "printf");
  if 2 + 2 = 4 then
		LibraryCall(printf_token, "Yahoo!");
    if 2 + 1 = 4 then
       LibraryCall(printf_token, "I got that summertime, summertime sadness..")
	else
		LibraryCall(printf_token, "I'm all geared up!");
	LibraryUnload(lib_token);
end.
