-- source-links.lua
-- The name of a file or folder of the project written as code and not yet
-- within a link, e.g., `SMS++/include/Block.h`, becomes a link to it in the
-- GitLab repository of its submodule, as hooks.py does on the web site. The
-- files are those that git tracks under SMSPP_ROOT (by default the umbrella
-- of this file), with the submodules checked out. Run before break-code.lua,
-- which leaves the link alone.

local REPO = 'https://gitlab.com/smspp/smspp-project' -- the umbrella
local BRANCH = 'develop' -- the branch published
local here = PANDOC_SCRIPT_FILE:match('(.*/)') or './'
local ROOT = os.getenv('SMSPP_ROOT') or (here .. '../../')

-- the folder of each submodule on GitLab -> the URL of its repository
local repos = { [''] = REPO }
do
  local f = io.open(ROOT .. '/.gitmodules', 'r')
  if f then
    local path
    for line in f:lines() do
      local k, v = line:match('^%s*(%w+) = (.-)%s*$')
      if k == 'path' then
        path = v
      elseif k == 'url' and path then
        local url = v:gsub('%.git$', '')
        if url:sub(1, 3) == '../' then
          url = REPO:match('(.*)/') .. url:sub(3)
        end
        if url:find('^https://gitlab%.com/') then
          repos[path] = url
        end
      end
    end
    f:close()
  end
end

-- the files that git tracks in the umbrella and in its submodules, and the
-- folders that hold them
local files, dirs = {}, {}
do
  local p = io.popen('git -C "' .. ROOT .. '" ls-files --recurse-submodules')
  if p then
    for name in p:lines() do
      files[name] = true
      local d = name:match('(.*)/')
      while d and not dirs[d] do
        dirs[d] = true
        d = d:match('(.*)/')
      end
    end
    p:close()
  end
end

local function url_of(path)
  if not path:find('/') or path:find('%.%.') or
     not path:match('^[%w_.+][%w_.+/%-]*$') then
    return nil
  end
  local name = path:gsub('/$', '')
  local kind = files[name] and 'blob' or (dirs[name] and 'tree')
  if not kind then
    return nil
  end
  local sub = ''
  for s in pairs(repos) do
    if #s > #sub and (name == s or name:sub(1, #s + 1) == s .. '/') then
      sub = s
    end
  end
  local rest = name:sub(#sub + 1):gsub('^/', '')
  local url = repos[sub] .. '/-/' .. kind .. '/' .. BRANCH
  return rest ~= '' and (url .. '/' .. rest) or url
end

local function code(el)
  local url = url_of(el.text)
  if not url then
    return nil
  end
  return pandoc.Link({ el }, url), false
end

-- top-down, so that a code span already within a link stays as it is
return {
  {
    traverse = 'topdown',
    Link = function(el) return el, false end,
    Code = code,
  }
}
