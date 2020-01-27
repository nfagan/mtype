function redraw_cb(h, eventdata)
h = getparent(h);
opt = getappdata(h, 'opt');
cfg = getappdata(h, 'cfg');

figure(h); % ensure that the calling figure is in the front

%fprintf('redrawing with viewmode %s\n', cfg.viewmode);

cfg.channelclamped = ft_getopt(cfg, 'channelclamped');

% temporarily store the originally selected list of channels
userchan    = cfg.channel;

% add the clamped channels (cfg.channelclamped) at the end of the list
cfg.channel = setdiff(cfg.channel, cfg.channelclamped);
cfg.channel =   union(cfg.channel, cfg.channelclamped);

% if the number of channels changes, includes past channels
if numel(cfg.channel)<numel(userchan) + numel(cfg.channelclamped)
  addchan = numel(userchan) + numel(cfg.channelclamped) - numel(cfg.channel); % Gets the number of channels to add.
  lastchan = min(match_str(opt.hdr.label, cfg.channel)) - 1; % Gets the previous channel.
  
  % Adds up to n more channels.
  cfg.channel = union(opt.hdr.label(max(lastchan - addchan + 1, 1): lastchan), cfg.channel);
end

begsample = opt.trlvis(opt.trlop, 1);
endsample = opt.trlvis(opt.trlop, 2);
offset    = opt.trlvis(opt.trlop, 3);
chanindx  = match_str(opt.hdr.label, cfg.channel);

% parse opt.changedchanflg, and rese
changedchanflg = false;
if opt.changedchanflg
  changedchanflg = true; % trigger for redrawing channel labels and preparing layout again (see bug 2065 and 2878)
  opt.changedchanflg = false;
end

if ~isempty(opt.event) && isstruct(opt.event)
  % select only the events in the current time window
  event     = opt.event;
  evtsample = [event(:).sample];
  event     = event(evtsample>=begsample & evtsample<=endsample);
else
  event = [];
end

if isempty(opt.orgdata)
  dat = ft_read_data(cfg.datafile, 'header', opt.hdr, 'begsample', begsample, 'endsample', endsample, 'chanindx', chanindx, 'checkboundary', ~istrue(cfg.continuous), 'dataformat', cfg.dataformat, 'headerformat', cfg.headerformat);
else
  dat = ft_fetch_data(opt.orgdata, 'header', opt.hdr, 'begsample', begsample, 'endsample', endsample, 'chanindx', chanindx, 'allowoverlap', istrue(cfg.allowoverlap));
end
art = ft_fetch_data(opt.artdata, 'begsample', begsample, 'endsample', endsample);

% convert the data to another numeric precision, i.e. double, single or int32
if ~isempty(cfg.precision)
  dat = cast(dat, cfg.precision);
end

% apply preprocessing and determine the time axis
[dat, lab, tim] = preproc(dat, opt.hdr.label(chanindx), offset2time(offset, opt.fsample, size(dat,2)), cfg.preproc);

% add NaNs to data for plotting purposes. NaNs will be added when requested horizontal scaling is longer than the data.
nsamplepad = opt.nanpaddata(opt.trlop);
if nsamplepad>0
  dat = [dat NaN(numel(lab), opt.nanpaddata(opt.trlop))];
  tim = [tim linspace(tim(end),tim(end)+nsamplepad*mean(diff(tim)),nsamplepad)];  % possible machine precision error here
end

% make a single-trial data structure for the current data
opt.curdata.label      = lab;
opt.curdata.time{1}    = tim;
opt.curdata.trial{1}   = dat;
opt.curdata.hdr        = opt.hdr;
opt.curdata.fsample    = opt.fsample;
opt.curdata.sampleinfo = [begsample endsample];
% remove the local copy of the data fields
clear lab tim dat

fn = fieldnames(cfg);
tmpcfg = keepfields(cfg, fn(contains(fn, 'scale') | contains(fn, 'mychan')));
tmpcfg.parameter = 'trial';
opt.curdata = chanscale_common(tmpcfg, opt.curdata);

% make a local copy (again) of the data fields
lab = opt.curdata.label;
tim = opt.curdata.time{1};
dat = opt.curdata.trial{1};

% to assure current feature is plotted on top
ordervec = 1:length(opt.artdata.label);
if numel(opt.ftsel)==1
  ordervec(opt.ftsel) = [];
  ordervec(end+1) = opt.ftsel;
end

% FIXME speedup ft_prepare_layout
if strcmp(cfg.viewmode, 'butterfly')
  laytime = [];
  laytime.label = {'dummy'};
  laytime.pos = [0.5 0.5];
  laytime.width = 1;
  laytime.height = 1;
  opt.laytime = laytime;
else
  % this needs to be reconstructed if the channel selection changes
  if changedchanflg % trigger for redrawing channel labels and preparing layout again (see bug 2065 and 2878)
    tmpcfg = [];
    if strcmp(cfg.viewmode, 'component')
      tmpcfg.layout  = 'vertical';
    else
      tmpcfg.layout  = cfg.viewmode;
    end
    tmpcfg.channel = cfg.channel;
    tmpcfg.skipcomnt = 'yes';
    tmpcfg.skipscale = 'yes';
    tmpcfg.showcallinfo = 'no';
    opt.laytime = ft_prepare_layout(tmpcfg, opt.orgdata);
  end
end

% determine the position of the channel/component labels relative to the real axes
% FIXME needs a shift to the left for components
labelx = opt.laytime.pos(:,1) - opt.laytime.width/2 - 0.01;
labely = opt.laytime.pos(:,2);

% determine the total extent of all virtual axes relative to the real axes
ax(1) = min(opt.laytime.pos(:,1) - opt.laytime.width/2);
ax(2) = max(opt.laytime.pos(:,1) + opt.laytime.width/2);
ax(3) = min(opt.laytime.pos(:,2) - opt.laytime.height/2);
ax(4) = max(opt.laytime.pos(:,2) + opt.laytime.height/2);
% add white space to bottom and top so channels are not out-of-axis for the majority
% NOTE: there is a second spot where this is done below, specifically for viewmode = component (also need to be here), which should be kept the same as this
if any(strcmp(cfg.viewmode,{'vertical', 'component'}))
  % determine amount of vertical padding using cfg.verticalpadding
  if ~isnumeric(cfg.verticalpadding) && strcmp(cfg.verticalpadding, 'auto')
    % determine amount of padding using the number of channels
    if numel(cfg.channel)<=6
      wsfac = 0;
    elseif numel(cfg.channel)>6 && numel(cfg.channel)<=10
      wsfac = 0.01 *  (ax(4)-ax(3));
    else
      wsfac = 0.02 *  (ax(4)-ax(3));
    end
  else
    wsfac = cfg.verticalpadding * (ax(4)-ax(3));
  end
  ax(3) = ax(3) - wsfac;
  ax(4) = ax(4) + wsfac;
end
axis(ax)


% determine a single local axis that encompasses all channels
% this is in relative figure units
opt.hpos   = (ax(1)+ax(2))/2;
opt.vpos   = (ax(3)+ax(4))/2;
opt.width  = ax(2)-ax(1);
opt.height = ax(4)-ax(3);

% these determine the scaling inside the virtual axes
% the hlim will be in seconds, the vlim will be in Tesla or Volt
opt.hlim = [tim(1) tim(end)];
opt.vlim = cfg.ylim;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% fprintf('plotting artifacts...\n');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
delete(findobj(h, 'tag', 'artifact'));

for j = ordervec
  tmp = diff([0 art(j,:) 0]);
  artbeg = find(tmp==+1);
  artend = find(tmp==-1) - 1;
  
  for k=1:numel(artbeg)
    xpos = [tim(artbeg(k)) tim(artend(k))] + ([-.5 +.5]./opt.fsample);
    ft_plot_box([xpos -1 1], 'facecolor', opt.artifactcolors(j,:), 'facealpha', cfg.artifactalpha, 'edgecolor', 'none', 'tag', 'artifact', 'hpos', opt.hpos, 'vpos', opt.vpos, 'width', opt.width, 'height', opt.height, 'hlim', opt.hlim, 'vlim', [-1 1]);
  end
end % for each of the artifact channels

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% fprintf('plotting events...\n');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
delete(findobj(h, 'tag', 'event'));

if strcmp(cfg.plotevents, 'yes')
  if any(strcmp(cfg.viewmode, {'butterfly', 'component', 'vertical'}))
    
    if strcmp(cfg.ploteventlabels , 'colorvalue') && ~isempty(opt.event)
      eventlabellegend = [];
      for iType = 1:length(opt.eventtypes)
        eventlabellegend = [eventlabellegend sprintf('%s = %s\n',opt.eventtypes{iType},opt.eventtypecolorlabels{iType})];
      end
      fprintf(eventlabellegend);
    end
    
    % save stuff to able to shift event labels downwards when they occur at the same time-point
    eventcol = cell(1,numel(event));
    eventstr = cell(1,numel(event));
    eventtim = NaN(1,numel(event));
    concount = NaN(1,numel(event));
    
    % gather event info and plot lines
    for ievent = 1:numel(event)
      try
        if strcmp(cfg.ploteventlabels , 'type=value')
          eventcol{ievent} = 'k';
          if isempty(event(ievent).value)
            eventstr{ievent} = sprintf('%s', event(ievent).type);
          else
            eventstr{ievent} = sprintf('%s = %s', event(ievent).type, num2str(event(ievent).value)); % value can be both number and string
          end
        elseif strcmp(cfg.ploteventlabels , 'colorvalue')
          eventcol{ievent} = opt.eventtypescolors(match_str(opt.eventtypes, event(ievent).type),:);
          eventstr{ievent} = sprintf('%s', num2str(event(ievent).value)); % value can be both number and string
        end
      catch
        eventcol{ievent} = 'k';
        eventstr{ievent} = 'unknown';
      end
      
      % compute the time of the event
      eventtim(ievent) = (event(ievent).sample-begsample)/opt.fsample + opt.hlim(1);
      
      lh = ft_plot_line([eventtim(ievent) eventtim(ievent)], [-1 1], 'tag', 'event', 'color', eventcol{ievent}, 'hpos', opt.hpos, 'vpos', opt.vpos, 'width', opt.width, 'height', opt.height, 'hlim', opt.hlim, 'vlim', [-1 1]);
      
      % store this data in the line object so that it can be displayed in the
      % data cursor (see subfunction datacursortext below)
      setappdata(lh, 'ft_databrowser_linetype', 'event');
      setappdata(lh, 'ft_databrowser_eventtime', eventtim(ievent));
      setappdata(lh, 'ft_databrowser_eventtype', event(ievent).type);
      setappdata(lh, 'ft_databrowser_eventvalue', event(ievent).value);
      
      % count the consecutive occurrence of each time point, this is used for the vertical shift of the event label
      concount(ievent) = sum(eventtim(ievent)==eventtim(1:ievent-1));
      
      % plot the event label
      ft_plot_text(eventtim(ievent), 0.9-concount(ievent)*.06, eventstr{ievent}, 'tag', 'event', 'Color', eventcol{ievent}, 'hpos', opt.hpos, 'vpos', opt.vpos, 'width', opt.width, 'height', opt.height, 'hlim', opt.hlim, 'vlim', [-1 1], 'FontSize', cfg.fontsize, 'FontUnits', cfg.fontunits, 'horizontalalignment', 'left');
    end
    
  end % if viewmode appropriate for events
end % if user wants to see event marks

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%fprintf('plotting data...\n');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
delete(findobj(h, 'tag', 'timecourse'));
delete(findobj(h, 'tag', 'identify'));

% not removing channel labels, they cause the bulk of redrawing time for the slow text function (note, interpreter = none hardly helps)
% warning, when deleting the labels using the line below, one can easily tripple the excution time of redrawing in viewmode = vertical (see bug 2065)
%delete(findobj(h, 'tag', 'chanlabel'));

if strcmp(cfg.viewmode, 'butterfly')
  set(gca, 'ColorOrder',opt.chancolors(chanindx,:)) % plot vector does not clear axis, therefore this is possible
  ft_plot_vector(tim, dat, 'box', false, 'tag', 'timecourse', 'hpos', opt.laytime.pos(1,1), 'vpos', opt.laytime.pos(1,2), 'width', opt.laytime.width(1), 'height', opt.laytime.height(1), 'hlim', opt.hlim, 'vlim', opt.vlim, 'linewidth', cfg.linewidth);
  set(gca, 'FontSize', cfg.axisfontsize, 'FontUnits', cfg.axisfontunits);
  
  % two ticks per channel
  yTick = sort([
    opt.laytime.pos(:,2)+(opt.laytime.height/2)
    opt.laytime.pos(:,2)+(opt.laytime.height/4)
    opt.laytime.pos(:,2)
    opt.laytime.pos(:,2)-(opt.laytime.height/4)
    opt.laytime.pos(:,2)-(opt.laytime.height/2)
    ]); % sort
  
  yTickLabel = {num2str(yTick.*range(opt.vlim) + opt.vlim(1))};
  
  set(gca, 'yTick', yTick, 'yTickLabel', yTickLabel)
  
elseif any(strcmp(cfg.viewmode, {'component', 'vertical'}))
  
  % determine channel indices into data outside of loop
  laysels = match_str(opt.laytime.label, opt.hdr.label);
  % delete old chan labels before renewing, if they need to be renewed
  if changedchanflg % trigger for redrawing channel labels and preparing layout again (see bug 2065 and 2878)
    delete(findobj(h, 'tag', 'chanlabel'));
  end
  for i = 1:length(chanindx)
    if strcmp(cfg.viewmode, 'component')
      color = 'k';
    else
      color = opt.chancolors(chanindx(i),:);
    end
    datsel = i;
    laysel = laysels(i);
    
    if ~isempty(datsel) && ~isempty(laysel)
      % only plot chanlabels when necessary
      if changedchanflg % trigger for redrawing channel labels and preparing layout again (see bug 2065 and 2878)
        % determine how many labels to skip in case of 'some'
        if opt.plotLabelFlag == 2 && strcmp(cfg.fontunits, 'points')
          % determine number of labels to plot by estimating overlap using current figure size
          % the idea is that figure height in pixels roughly corresponds to the amount of letters at cfg.fontsize (points) put above each other without overlap
          figheight = get(h, 'Position');
          figheight = figheight(4);
          labdiscfac = ceil(numel(chanindx) ./ (figheight ./ (cfg.fontsize+6))); % 6 added, so that labels are not too close together (i.e. overlap if font was 6 points bigger)
        else
          labdiscfac = 10;
        end
        if opt.plotLabelFlag == 1 || (opt.plotLabelFlag == 2 && mod(i,labdiscfac)==0)
          ft_plot_text(labelx(laysel), labely(laysel), opt.hdr.label(chanindx(i)), 'tag', 'chanlabel', 'HorizontalAlignment', 'right', 'FontSize', cfg.fontsize, 'FontUnits', cfg.fontunits, 'linewidth', cfg.linewidth);
          set(gca, 'FontSize', cfg.axisfontsize, 'FontUnits', cfg.axisfontunits);
        end
      end
      
      lh = ft_plot_vector(tim, dat(datsel, :), 'box', false, 'color', color, 'tag', 'timecourse', 'hpos', opt.laytime.pos(laysel,1), 'vpos', opt.laytime.pos(laysel,2), 'width', opt.laytime.width(laysel), 'height', opt.laytime.height(laysel), 'hlim', opt.hlim, 'vlim', opt.vlim, 'linewidth', cfg.linewidth);
      
      % store this data in the line object so that it can be displayed in the
      % data cursor (see subfunction datacursortext below)
      setappdata(lh, 'ft_databrowser_linetype', 'channel');
      setappdata(lh, 'ft_databrowser_label', opt.hdr.label(chanindx(i)));
      setappdata(lh, 'ft_databrowser_xaxis', tim);
      setappdata(lh, 'ft_databrowser_yaxis', dat(datsel,:));
    end
  end
  
  % plot yticks
  if length(chanindx)> 6
    % plot yticks at each label in case adaptive labeling is used (cfg.plotlabels = 'some')
    % otherwise, use the old ytick plotting based on hard-coded number of channels
    if opt.plotLabelFlag == 2
      if opt.plotLabelFlag == 2 && strcmp(cfg.fontunits, 'points')
        % determine number of labels to plot by estimating overlap using current figure size
        % the idea is that figure height in pixels roughly corresponds to the amount of letters at cfg.fontsize (points) put above each other without overlap
        figheight = get(h, 'Position');
        figheight = figheight(4);
        labdiscfac = ceil(numel(chanindx) ./ (figheight ./ (cfg.fontsize+2))); % 2 added, so that labels are not too close together (i.e. overlap if font was 2 points bigger)
      else
        labdiscfac = 10;
      end
      yTick = sort(labely(mod(chanindx,labdiscfac)==0), 'ascend'); % sort is required, yticks should be increasing in value
      yTickLabel = [];
    else
      if length(chanindx)>19
        % no space for yticks
        yTick = [];
        yTickLabel = [];
      elseif length(chanindx)> 6
        % one tick per channel
        yTick = sort([
          opt.laytime.pos(:,2)+(opt.laytime.height(laysel)/4)
          opt.laytime.pos(:,2)-(opt.laytime.height(laysel)/4)
          ]);
        yTickLabel = {[.25 .75] .* range(opt.vlim) + opt.vlim(1)};
      end
    end
  else
    % two ticks per channel
    yTick = sort([
      opt.laytime.pos(:,2)+(opt.laytime.height(laysel)/2)
      opt.laytime.pos(:,2)+(opt.laytime.height(laysel)/4)
      opt.laytime.pos(:,2)-(opt.laytime.height(laysel)/4)
      opt.laytime.pos(:,2)-(opt.laytime.height(laysel)/2)
      ]); % sort
    yTickLabel = {[.0 .25 .75 1] .* range(opt.vlim) + opt.vlim(1)};
  end
  yTickLabel = repmat(yTickLabel, 1, length(chanindx));
  set(gca, 'yTick', yTick, 'yTickLabel', yTickLabel);
  
else
  % the following is implemented for 2column, 3column, etcetera.
  % it also works for topographic layouts, such as CTF151
  
  % determine channel indices into data outside of loop
  laysels = match_str(opt.laytime.label, opt.hdr.label);
  
  for i = 1:length(chanindx)
    color = opt.chancolors(chanindx(i),:);
    datsel = i;
    laysel = laysels(i);
    
    if ~isempty(datsel) && ~isempty(laysel)
      
      lh = ft_plot_vector(tim, dat(datsel, :), 'box', false, 'color', color, 'tag', 'timecourse', 'hpos', opt.laytime.pos(laysel,1), 'vpos', opt.laytime.pos(laysel,2), 'width', opt.laytime.width(laysel), 'height', opt.laytime.height(laysel), 'hlim', opt.hlim, 'vlim', opt.vlim, 'linewidth', cfg.linewidth);
      
      % store this data in the line object so that it can be displayed in the
      % data cursor (see subfunction datacursortext below)
      setappdata(lh, 'ft_databrowser_linetype', 'channel');
      setappdata(lh, 'ft_databrowser_label', opt.hdr.label(chanindx(i)));
      setappdata(lh, 'ft_databrowser_xaxis', tim);
      setappdata(lh, 'ft_databrowser_yaxis', dat(datsel,:));
    end
  end
  
  % ticks are not supported with such a layout
  yTick = [];
  yTickLabel = [];
  yTickLabel = repmat(yTickLabel, 1, length(chanindx));
  set(gca, 'yTick', yTick, 'yTickLabel', yTickLabel);
  
end % if strcmp viewmode

if any(strcmp(cfg.viewmode, {'butterfly', 'component', 'vertical'}))
  nticks = 11;
  xTickLabel = cellstr(num2str( linspace(tim(1), tim(end), nticks)' , '%1.2f'))';
  xTick = linspace(ax(1), ax(2), nticks);
  if nsamplepad>0
    nlabindat = sum(linspace(tim(1), tim(end), nticks) < tim(end-nsamplepad));
    xTickLabel(nlabindat+1:end) = repmat({' '}, [1 nticks-nlabindat]);
  end
  set(gca, 'xTick', xTick, 'xTickLabel', xTickLabel)
  xlabel('time');
else
  set(gca, 'xTick', [], 'xTickLabel', [])
end

if strcmp(cfg.viewmode, 'component')
  
  % determine the position of each of the original channels for the topgraphy
  laychan = opt.layorg;
  
  % determine the position of each of the topographies
  laytopo.pos(:,1)  = opt.laytime.pos(:,1) - opt.laytime.width/2 - opt.laytime.height;
  laytopo.pos(:,2)  = opt.laytime.pos(:,2) + opt.laytime.height/2;
  laytopo.width     = opt.laytime.height;
  laytopo.height    = opt.laytime.height;
  laytopo.label     = opt.laytime.label;
  
  if ~isequal(opt.chanindx, chanindx)
    opt.chanindx = chanindx;
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % fprintf('plotting component topographies...\n');
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    delete(findobj(h, 'tag', 'topography'));
    
    [sel1, sel2] = match_str(opt.orgdata.topolabel, laychan.label);
    chanx = laychan.pos(sel2,1);
    chany = laychan.pos(sel2,2);
    
    if strcmp(cfg.compscale, 'global')
      for i=1:length(chanindx) % loop through all components to get max and min
        zmin(i) = min(opt.orgdata.topo(sel1,chanindx(i)));
        zmax(i) = max(opt.orgdata.topo(sel1,chanindx(i)));
      end
      
      if strcmp(cfg.zlim, 'maxmin')
        zmin = min(zmin);
        zmax = max(zmax);
      elseif strcmp(cfg.zlim, 'maxabs')
        zmax = max([abs(zmin) abs(zmax)]);
        zmin = -zmax;
      else
        ft_error('configuration option for component scaling could not be recognized');
      end
    end
    
    for i=1:length(chanindx)
      % plot the topography of this component
      laysel = match_str(opt.laytime.label, opt.hdr.label(chanindx(i)));
      chanz = opt.orgdata.topo(sel1,chanindx(i));
      
      if strcmp(cfg.compscale, 'local')
        % compute scaling factors here
        if strcmp(cfg.zlim, 'maxmin')
          zmin = min(chanz);
          zmax = max(chanz);
        elseif strcmp(cfg.zlim, 'maxabs')
          zmax = max(abs(chanz));
          zmin = -zmax;
        end
      end
      
      % scaling
      chanz = (chanz - zmin) ./  (zmax- zmin);
      
      % laychan is the actual topo layout, in pixel units for .mat files
      % laytopo is a vertical layout determining where to plot each topo, with one entry per component
      
      ft_plot_topo(chanx, chany, chanz, 'mask', laychan.mask, 'interplim', 'mask', 'outline', laychan.outline, 'tag', 'topography', 'hpos', laytopo.pos(laysel,1)-laytopo.width(laysel)/2, 'vpos', laytopo.pos(laysel,2)-laytopo.height(laysel)/2, 'width', laytopo.width(laysel), 'height', laytopo.height(laysel), 'gridscale', 45, 'isolines', cfg.contournum);
      
      %axis equal
      %drawnow
    end
    
    caxis([0 1]);
    
  end % if redraw_topo
  
  set(gca, 'yTick', [])
  
  ax(1) = min(laytopo.pos(:,1) - laytopo.width);
  ax(2) = max(opt.laytime.pos(:,1) + opt.laytime.width/2);
  ax(3) = min(opt.laytime.pos(:,2) - opt.laytime.height/2);
  ax(4) = max(opt.laytime.pos(:,2) + opt.laytime.height/2);
  % add white space to bottom and top so channels are not out-of-axis for the majority
  % NOTE: there is another spot above with the same code, which should be kept the same as this
  % determine amount of vertical padding using cfg.verticalpadding
  if ~isnumeric(cfg.verticalpadding) && strcmp(cfg.verticalpadding, 'auto')
    % determine amount of padding using the number of channels
    if numel(cfg.channel)<=6
      wsfac = 0;
    elseif numel(cfg.channel)>6 && numel(cfg.channel)<=10
      wsfac = 0.01 *  (ax(4)-ax(3));
    else
      wsfac = 0.02 *  (ax(4)-ax(3));
    end
  else
    wsfac = cfg.verticalpadding * (ax(4)-ax(3));
  end
  ax(3) = ax(3) - wsfac;
  ax(4) = ax(4) + wsfac;
  axis(ax)
end % plotting topographies

startim = tim(1);
if nsamplepad>0
  endtim = tim(end-nsamplepad);
else
  endtim = tim(end);
end

if ~strcmp(opt.trialviewtype, 'trialsegment')
  str = sprintf('%s %d/%d, time from %g to %g s', opt.trialviewtype, opt.trlop, size(opt.trlvis,1), startim, endtim);
else
  str = sprintf('trial %d/%d: segment: %d/%d , time from %g to %g s', opt.trllock, size(opt.trlorg,1), opt.trlop, size(opt.trlvis,1), startim, endtim);
end
title(str);

% possibly adds some responsiveness if the 'thing' is clogged
drawnow

% restore the originally selected list of channels, in case a set of
% additional fixed channels was selected
cfg.channel = userchan;

setappdata(h, 'opt', opt);
setappdata(h, 'cfg', cfg);
end % function redraw_cb