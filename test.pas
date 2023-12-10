program Hello;
var a, b : Integer; c, superstring : String;
begin
  write_str("Enter an integer: ");
  a := read_int();
  write_str("Enter an integer: ");
  b := read_int();
  write_int(a + b);
  write_char(chr(10));

  write_int(a - b);
  write_char(chr(10));
  if 2 + 2 = 4 then
    if 2 + 1 = 4 then
       write_int(a + b)
    else begin
       write_str("No dangling else! Hooray!");
       write_char(chr(10));
       write_int(a * b);
       write_char(chr(10));
    end;
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

  write_char(chr(10));
  for i := 0 to 5 do begin
     append(superstring, "O"); append(superstring, "le");
  end;
  append(superstring, "!");
  write_str(superstring);
  write_char(chr(10));
  
  while strlen(superstring) <> 0 do
  begin
    write_char(superstring[strlen(superstring)-1]);
    drop(superstring);
  end;
  write_char(chr(10));

  write_int(2 * 2 div 4 + 1 mod 8);
  write_char(chr(10));
end.
