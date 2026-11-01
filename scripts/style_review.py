#!/usr/bin/env python3
"""Posts on a merge request of GitLab the changes that put its files in the
style of SMS++, as suggestions that the author applies with one click.

The patch is the one that the job of ci/style.yml makes by running the
formatters on the files of the merge request (git diff after them). A hunk
whose lines are all among the ones that the merge request adds becomes a
discussion on the first of them, with a suggestion that replaces them with
the formatted ones; the other hunks (e.g., of a configuration, which is
formatted whole) go together in one note, as a patch. The notes of the last
run are deleted first, so that a merge request carries only the ones of its
last pipeline; they are told by a marker in their body.

Usage: style_review.py [--dry-run] PATCH BASE
The variables of a merge-request pipeline say where to post (CI_API_V4_URL,
CI_PROJECT_ID, CI_MERGE_REQUEST_IID) and SMSPP_STYLE_TOKEN is the token of
the user that posts (a project access token with the role Reporter and the
scope api). With --dry-run nothing is posted, and what would be is printed.
"""

import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

MARKER = '<!-- smspp-style -->'
HEAD = (MARKER + '\n**Style of SMS++**: this change puts the lines in the '
        'style that the pipeline checks [see the page Code style of the wiki '
        'of the umbrella].\n\n')


def hunks(patch):
    """The hunks of the patch: (path, first old line, old lines, new
    lines), the lines without their +, - or space."""
    res = []
    path = None
    lines = patch.split('\n')
    k = 0
    while k < len(lines):
        line = lines[k]
        if line.startswith('+++ '):
            path = line[4:]
            path = path[2:] if path.startswith('b/') else path
        m = re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@', line)
        if m and path:
            start = int(m.group(1))
            old, new = [], []
            k += 1
            while k < len(lines) and lines[k][:1] in (' ', '-', '+', '\\'):
                t = lines[k]
                if t[:1] in (' ', '-'):
                    old.append(t[1:])
                if t[:1] in (' ', '+'):
                    new.append(t[1:])
                k += 1
            # the context around the change is kept out of the suggestion
            while old and new and old[0] == new[0]:
                old.pop(0)
                new.pop(0)
                start += 1
            while old and new and old[-1] == new[-1]:
                old.pop()
                new.pop()
            res.append((path, start, old, new))
            continue
        k += 1
    return res


def added_lines(base, path):
    """The numbers of the lines of path at HEAD that the merge request adds
    or changes."""
    out = subprocess.run(['git', 'diff', '-U0', base, 'HEAD', '--', path],
                         capture_output=True, text=True).stdout
    lines = set()
    for m in re.finditer(r'^@@ -\S+ \+(\d+)(?:,(\d+))? @@', out, re.M):
        first, count = int(m.group(1)), int(m.group(2) or 1)
        lines.update(range(first, first + count))
    return lines


class GitLab:
    def __init__(self, dry):
        self.dry = dry
        self.api = os.environ.get('CI_API_V4_URL', '')
        self.project = os.environ.get('CI_PROJECT_ID', '')
        self.iid = os.environ.get('CI_MERGE_REQUEST_IID', '')
        self.token = os.environ.get('SMSPP_STYLE_TOKEN', '')

    def call(self, method, path, data=None):
        url = '%s/projects/%s/merge_requests/%s%s' % (
            self.api, self.project, self.iid, path)
        body = None
        if data is not None:
            body = urllib.parse.urlencode(data).encode()
        req = urllib.request.Request(url, data=body, method=method,
                                     headers={'PRIVATE-TOKEN': self.token})
        with urllib.request.urlopen(req) as r:
            text = r.read().decode()
        return json.loads(text) if text else None

    def notes(self):
        """The ids of the discussions and of the notes of the last run."""
        res = []
        page = 1
        while True:
            ds = self.call('GET', '/discussions?per_page=100&page=%d' % page)
            if not ds:
                return res
            for d in ds:
                n = d['notes'][0]
                if MARKER in n['body']:
                    res.append(n['id'])
            page += 1

    def post(self, data):
        if self.dry:
            print(json.dumps(data, indent=1))
            return True
        try:
            self.call('POST', '/discussions', data)
            return True
        except urllib.error.HTTPError:
            return False


def main(argv):
    dry = '--dry-run' in argv
    args = [a for a in argv if a != '--dry-run']
    patch = open(args[0], encoding='utf-8').read()
    base = args[1]
    gl = GitLab(dry)
    if not dry and not (gl.token and gl.iid):
        print('style-review: no SMSPP_STYLE_TOKEN or no merge request, '
              'nothing posted')
        return 0
    refs = None
    if not dry:
        for nid in gl.notes():
            gl.call('DELETE', '/notes/%d' % nid)
        refs = gl.call('GET', '')['diff_refs']
    rest = []
    added = {}
    for path, start, old, new in hunks(patch):
        if path not in added:
            added[path] = added_lines(base, path)
        span = range(start, start + max(len(old), 1))
        if old and all(n in added[path] for n in span):
            body = HEAD + '```suggestion:-0+%d\n%s\n```' % (
                len(old) - 1, '\n'.join(new))
            data = {'body': body,
                    'position[position_type]': 'text',
                    'position[new_path]': path,
                    'position[old_path]': path,
                    'position[new_line]': start}
            if refs:
                data['position[base_sha]'] = refs['base_sha']
                data['position[start_sha]'] = refs['start_sha']
                data['position[head_sha]'] = refs['head_sha']
            if gl.post(data):
                continue
        rest.append((path, start, old, new))
    if rest:
        text = []
        for path, start, old, new in rest:
            text.append('%s, line %d:\n```diff\n%s\n```' % (
                path, start, '\n'.join(['-' + x for x in old] +
                                       ['+' + x for x in new])))
        gl.post({'body': HEAD + 'The changes that cannot be suggested on '
                 'the lines of the merge request (e.g., in a configuration, '
                 'which is formatted whole):\n\n' + '\n\n'.join(text) +
                 '\n\nThe whole patch is the artifact `style.patch` of the '
                 'job `style`.'})
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
