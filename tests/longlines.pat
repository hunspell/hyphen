ISO8859-1
LEFTHYPHENMIN 1
RIGHTHYPHENMIN 1
% Check whether characters over MAX_CHARS are not treated as new line
% This test is valid as long as MAX_CHARS is 100
%
% Following pattern should result in a=bc=d hyphenation
a1b2c1d
% and should not be overriden by pattern from too long comment (over MAX_CHARS characters)
%|------------------------------ this part is 100 characters long --------------------------------|a8b9c8d
