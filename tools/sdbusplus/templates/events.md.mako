${"##"} Events

% if events.errors:
${"###"} Errors

% for e in events.errors:
${events.render(loader, "event.md.mako", event=e)}
% endfor

% endif
% if events.events:
${"###"} Events

% for e in events.events:
${events.render(loader, "event.md.mako", event=e)}
% endfor

% endif
