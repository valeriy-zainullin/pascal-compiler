program Hello;
var a, b : Integer; c : String;
begin
  write_str("Enter an integer: ");
  a := read_int();
  write_str("Enter an integer: ");
  b := read_int();
  write_int(a + b);
  write_char(chr(10));

  if 2 + 2 = 4 then
    write_int(a - b);
    if 2 + 1 = 4 then
       write_int(a + b);
  write_char(chr(10));  
  write_str("Enter a string: ");
  c := read_str();
  append(c, 'D');
  append(c, c[0]);
  write_str(c);
  write_char(chr(10));
  write_int(2 * strlen(c));
  write_char(chr(10));

  if c[strlen(c) - 2] = chr(ord('D')) then
    write_str("We win!");
end.
