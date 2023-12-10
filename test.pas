program Hello;
var a, b : Integer; c : String;
begin
  a := read_int();
  b := read_int();
  write_int(a + b);
  write_char(chr(13));

  if 2 + 2 = 4 then
    write_int(a - b);
    if 2 + 1 = 4 then
       write_int(a + b);
	else
		write_int(a * b);

  c := read_str();
  append(c, 'a');
  write_str(c);
  write_char(chr(13));
end.
