${"####"} ${event.name}

% if event.redfish_map:
${event.redfish_map}
% elif event.languages["en"].description:
${event.languages["en"].description}
% else:
${event.languages["en"].message}
% endif

% if event.is_error:
- severity: `${event.severity}`
- errno: `${event.errno}`
% endif
% if event.metadata:
- metadata:
% for m in event.metadata:
  - `${m.SNAKE_CASE}` as `${m.typeName}` \
% if m.primary:
**[PRIMARY]**
% else:

% endif
% endfor
% endif
