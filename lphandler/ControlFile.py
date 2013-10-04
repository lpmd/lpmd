#
#
#

Ignorable = lambda z: z.strip() == ''
RemoveComment = lambda z: z[0:z.find('#')] if '#' in z else z

s_syntax = 'module {module}'
p_syntax = s_syntax+' start end each'
m_syntax = 'properties start end each output'
a_syntax = p_syntax+' [ *over filtermodule {filtermodule} inverse except ]'
f_syntax = a_syntax.replace(p_syntax, p_syntax+' except')

oneliners = { 'cell crystal': 'a b c alpha beta gamma scale', 'cell vector': 'ax ay az bx by bz cx cy cz scale', 
              'cell cubic': 'a scale', 'input': p_syntax, 'output': p_syntax, 'steps': 'number', 'set': 'key value', 
              'periodic': 'px py pz', 'monitor': m_syntax, 'average': m_syntax, 'prepare': s_syntax, 
              'cellmanager': 'module', 'potential': 'module species1 species2', 'integrator': 'module start', 
              'apply': a_syntax, 'property': a_syntax, 'visualize': a_syntax, 'filter': a_syntax }

class Statement:

    def __init__(self, options, positional=None, extended=None):
        self.options, self.positional, self.extended = options, positional, extended

class UseBlock:

    def __init__(self, name, alias, options):
        self.name, self.alias, self.options = name, alias, options

class ControlFile(dict):
    def __init__(self, path=None): (self.Read(path) if path != None else None)

    def MatchSyntax(self, syntax, tokens):
        tokenlist, args, kwargs, usedkw, posargs, extmatched = tokens, list(), dict(), list(), dict(), None
        if '[' in syntax and ']' in syntax:
           extsyntax = syntax[syntax.find('[')+1:-1]
           condtoken = extsyntax.split().pop(0)[1:]
           extsyntax, syntax = ' '.join(extsyntax.split()[1:]), syntax[0:syntax.find('[')]
           if condtoken in tokens: 
              extmatched = self.MatchSyntax(extsyntax, tokens[tokens.index(condtoken)+1:])
              tokenlist = tokens[0:tokens.index(condtoken)]
        while tokenlist:
              token = tokenlist.pop(0)
              if '=' in token:                                          # keyword parameter
                 tp = token.partition('=')
                 kwargs[tp[0]] = tp[2]
                 usedkw.append(tp[0])
              else: args.append(token)                                  # positional parameter
        sspl = [z for z in syntax.split() if z not in usedkw]
        while args:
              if '{' in sspl[0] and '}' in sspl[0]:
                 if sspl[0][1:-1] not in posargs: posargs[sspl[0][1:-1]] = list()
                 posargs[sspl[0][1:-1]].append(args.pop(0))
              else: kwargs[sspl.pop(0)] = args.pop(0)
        return Statement(kwargs, positional=posargs, extended=extmatched)

    def Read(self, path):
        self.path = path
        self.ReadString(open(path, 'r').read())

    def ReadString(self, ss):
        lines = [z.strip() for z in (RemoveComment(w) for w in ss.split('\n')) if not Ignorable(z)]
        while lines:
              lspl = lines.pop(0).split()
              head, rest = lspl[0], lspl[1:]
              if head not in self: self[head] = list()
              if head in oneliners: self[head].append(self.MatchSyntax(oneliners[head], rest))
              elif ' '.join(lspl[0:2]) in oneliners: self[head].append(self.MatchSyntax(oneliners[' '.join(lspl[0:2])], rest[1:]))
              elif head == 'use':
                   plugin_name = lspl[1]
                   plugin_alias = lspl[3] if len(lspl) > 2 else plugin_name
                   blockdata = list()
                   while head != 'enduse':
                         lspl = lines.pop(0).split()
                         head = lspl[0]
                         if head != 'enduse': blockdata.append(lspl)
                   self['use'].append(UseBlock(plugin_name, plugin_alias, dict((z[0], z[1]) for z in blockdata)))

