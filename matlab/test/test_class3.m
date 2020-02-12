function DrawFormatedText

% Init cursor position:
xp = sx;
yp = sy;

minx = inf;
miny = inf;
maxx = 0;
maxy = 0;

if bjustify == 1
    % Pad to line width of a line of 'wrapat' X'es:
    padwidth = RectWidth(Screen('TextBounds', win, char(repmat('X', 1, wrapat)), [], [], 1, righttoleft));
end

if bjustify == 2 || bjustify == 3
    % Iterate over whole text string and find widest
    % text line. Use it as reference for padding:
    backuptext = tstring;

    % No clipping allowed in this opmode:
    disableClip = 1;

    % Iterate:
    padwidth = 0;
    while ~isempty(tstring)
        % Find next substring to process:
        crpositions = strfind(char(tstring), char(10));
        if ~isempty(crpositions)
            curstring = tstring(1:min(crpositions)-1);
            tstring = tstring(min(crpositions)+1:end);
        else
            curstring = tstring;
            tstring =[];
        end

        if ~isempty(curstring)
            padwidth = max(padwidth, RectWidth(Screen('TextBounds', win, curstring, [], [], 1, righttoleft)));
        end
    end

    % Restore original string for further processing:
    tstring = backuptext;
end

% Parse string, break it into substrings at line-feeds:
while ~isempty(tstring)
    deltaboxX = 0;
    deltaboxY = 0;

    % Find next substring to process:
    crpositions = strfind(char(tstring), char(10));
    if ~isempty(crpositions)
        curstring = tstring(1:min(crpositions)-1);
        tstring = tstring(min(crpositions)+1:end);
        dolinefeed = 1;
    else
        curstring = tstring;
        tstring =[];
        dolinefeed = 0;
    end

    % tstring contains the remainder of the input string to process in next
    % iteration, curstring is the string we need to draw now.

    % Perform crude clipping against upper and lower window borders for this text snippet.
    % If it is clearly outside the window and would get clipped away by the renderer anyway,
    % we can safe ourselves the trouble of processing it:
    if disableClip || ((yp + theight >= winRect(RectTop)) && (yp - theight <= winRect(RectBottom)))
        % Inside crude clipping area. Need to draw.
        noclip = 1;
    else
        % Skip this text line draw call, as it would be clipped away
        % anyway.
        noclip = 0;
        dolinefeed = 1;
    end

    % Any string to draw?
    if ~isempty(curstring) && noclip
        % Cast curstring back to the class of the original input string, to
        % make sure special unicode encoding (e.g., double()'s) does not
        % get lost for actual drawing:
        curstring = cast(curstring, stringclass);

        % Need bounding box?
        if xcenter || flipHorizontal || flipVertical || rjustify
            % Compute text bounding box for this substring:
            [bbox, refbbox] = Screen('TextBounds', win, curstring, 0, 0, 1, righttoleft);
            deltaboxX = refbbox(RectLeft) - bbox(RectLeft);
            deltaboxY = refbbox(RectTop) - bbox(RectTop);
        end

        % Horizontally centered output required?
        if xcenter
            % Yes. Compute dh, dv position offsets to center it in the center of window.
            [rect,dh] = CenterRect(bbox, winRect); %#ok<ASGLU>
            % Set drawing cursor to horizontal x offset:
            xp = dh - deltaboxX;
        end

        % Horizontally centered output of the block containing the full text string as a whole required?
        if bjustify == 3
            rect = CenterRect([0, 0, padwidth, 1], winRect);
            xp = rect(RectLeft);
        end

        % Right justified (aligned) output required?
        if rjustify
            xp = winRect(RectRight) - RectWidth(bbox);
        end

        if flipHorizontal || flipVertical
            if bjustify
                warning('Text justification to wrapat''th right column border not supported for flipHorizontal or flipVertical text drawing.');
            end

            textbox = OffsetRect(bbox, xp, yp);
            [xc, yc] = RectCenterd(textbox);

            % Make a backup copy of the current transformation matrix for later
            % use/restoration of default state:
            Screen('glPushMatrix', win);

            % Translate origin into the geometric center of text:
            Screen('glTranslate', win, xc, yc, 0);

            % Apple a scaling transform which flips the direction of x-Axis,
            % thereby mirroring the drawn text horizontally:
            if flipVertical
                Screen('glScale', win, 1, -1, 1);
            end

            if flipHorizontal
                Screen('glScale', win, -1, 1, 1);
            end

            % We need to undo the translations...
            Screen('glTranslate', win, -xc, -yc, 0);
            [nx ny] = Screen('DrawText', win, curstring, xp, yp, color, [], yPosIsBaseline, righttoleft);
            Screen('glPopMatrix', win);
        else
            % Block justification (align to left border and a right border at 'wrapat' columns)?
            if ismember(bjustify, [1,2])
                % Calculate required amount of padding in pixels:
                strwidth = padwidth - RectWidth(Screen('TextBounds', win, curstring(~isspace(curstring)), [], [], yPosIsBaseline, righttoleft));
                padpergapneeded = length(find(isspace(curstring)));
                % Padding needed and possible?
                if (padpergapneeded > 0) && (strwidth > 0)
                    % Required padding less than padthresh fraction of total
                    % width? If not we skip justification, as it would lead to
                    % ridiculous looking results:
                    if strwidth < padwidth * padthresh
                        % For each isolated blank in the text line, insert
                        % padpergapneeded pixels of blank space:
                        padpergapneeded = strwidth / padpergapneeded;
                    else
                        padpergapneeded = blankwidth;
                    end
                else
                    padpergapneeded = 0;
                end

                % Render text line word by word, adding padpergapneeded pixels of blank space
                % between consecutive words, to evenly distribute the padding space needed:
                [wordup, remstring] = strtok(curstring);
                cxp = xp;
                while ~isempty(wordup)
                    [nx ny] = Screen('DrawText', win, wordup, cxp, yp, color, [], yPosIsBaseline, righttoleft);
                    if ~isempty(remstring)
                        nx = nx + padpergapneeded;
                        cxp = nx;
                    end
                    [wordup, remstring] = strtok(remstring);
                end
            else
                [nx ny] = Screen('DrawText', win, curstring, xp, yp, color, [], yPosIsBaseline, righttoleft);
            end
        end
    else
        % This is an empty substring (pure linefeed). Just update cursor
        % position:
        nx = xp;
        ny = yp;
    end

    % Update bounding box:
    minx = min([minx , xp + deltaboxX, nx]);
    maxx = max([maxx , xp, nx + deltaboxX]);
    miny = min([miny , yp, ny]);
    maxy = max([maxy , yp, ny]);

    % Linefeed to do?
    if dolinefeed
        % Update text drawing cursor to perform carriage return:
        if ~xcenter && ~rjustify
            xp = sx;
        end
        yp = ny + theight;
    else
        % Keep drawing cursor where it is supposed to be:
        xp = nx;
        yp = ny;
    end
    % Done with substring, parse next substring.
end