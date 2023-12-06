program Hello;
begin
	LibraryLoad("glibc.so");
  if 2 + 2 = 4 then
		LibraryCall("printf", "Yahoo!");
    if 2 + 1 = 4 then
       LibraryCall("printf", "I got that summertime, summertime sadness..")
	else
		LibraryCall("printf", "I'm all geared up!");
	LibraryUnLoad("glibc.so");
end.
