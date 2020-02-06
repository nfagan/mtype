if 1
  z = 11;
end

if 1
  
[c, ms, h, names] = multcompare( stats, 'display', 'off' );
cg = arrayfun( @(x) names(x) );
y = arrayfun( @(x) z );

end