%{

A comment block begins with a line whose first non-whitespace character
sequence is `%{`, followed by a new-line (and possibly with whitespace in
between). A comment block ends with a line whose first non-whitespace
character is `%}`, followed by either a new-line or the end of file (and
possibly with whitespace in between).

In a comment block, if the first non-whitespace character sequence is `@t`,
then this marks the start of a type annotation. If the next non-whitespace
(and non-newline) character sequence is `begin`, then, while the `begin`
block and comment block remain unterminated, identifiers will be converted
to typing keywords as appropriate. Nested comment blocks in `begin` are not
allowed.

A single line comment `% @t` cannot include a `begin` keyword, and will be
terminated with a new line or end of file.

%{
@t begin
end
%}

%}

%{
@TODO: 
%}