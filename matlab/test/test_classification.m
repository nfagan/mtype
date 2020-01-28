function test_classification()
  x = nest(1);
  disp(x);
  
  function parsestruct = nest(parsestruct)
    import shared_utils.general.parsestruct;
    y = parsestruct();
  end
end