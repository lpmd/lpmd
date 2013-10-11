#
#
#

from optparse import OptionParser

KeyValueString = lambda x: ('%s %s' % (x[0], x[2]))

def ParsePluginCall(opt):
    name, dummy, options = opt.partition(':')
    return ('module=%s %s' % (name, ' '.join(options.split(','))))

def ParsePluginBlock(opt):
    name, dummy, options = opt.partition(':')
    useblock = ('use %s\n' % name)+'\n'.join(KeyValueString(z.partition('=')) for z in options.split(','))+'\nenduse'
    return (name, useblock)

class QuickMode:

    def __init__(self, use_hint):
        self.use_hint, self.op = use_hint, OptionParser()
        self.arguments = list()
        self.op.add_option('-v', '--verbose', action='store_true')
        self.op.add_option('-r', '--replace-cell', action='store_true')
        self.op.add_option('-p', '--pluginhelp')
        self.op.add_option('-T', '--test-plugin')
        self.op.add_option('-O', '--option')
        self.op.add_option('-S', '--scale')
        self.op.add_option('-i', '--input')
        self.op.add_option('-o', '--output')
        self.op.add_option('-u', '--use')
        self.op.add_option('-c', '--cellmanager')
        self.op.add_option('-L', '--lengths')
        self.op.add_option('-P', '--periodic')
        self.op.add_option('-A', '--angles')
        self.op.add_option('-V', '--vector')
        self.op.add_option('-g', '--grid')

    def GenerateControl(self):
        (options, args), control, scale = self.op.parse_args(), '', ''
        self.arguments = args
        if options.pluginhelp: return ('set help_plugin %s\n' % options.pluginhelp)
        if options.test_plugin: return ('set test_plugin %s\n' % options.test_plugin)
        a, b, c, alpha, beta, gamma = '1', '1', '1', '90', '90', '90'
        if options.scale: scale = ('scale=%s' % options.scale)
        if options.angles: alpha, beta, gamma = options.angles.split(',')
        if options.verbose: control += 'set verbose true\n'
        if options.replace_cell: control += 'set replacecell true\n'
        if options.option: control += ('\n'.join('set '+KeyValueString(z.partition('=')) for z in options.option.split(',')))+'\n'
        if options.periodic: control += ('periodic %s %s %s\n' % options.periodic.split(','))
        if options.grid: control += ('set grid %s\n' % options.grid)
        if options.vector: 
           control += ('cell vector ax=%s ay=%s az=%s bx=%s by=%s bz=%s cx=%s cy=%s cz=%s' % tuple(options.vector.split(',')))
           control += (' %s\n' % scale)
        elif options.lengths:
           if ',' not in options.lengths: 
              a = options.lengths
              b, c = a, a
           else: a, b, c = options.lengths.split(',')
           control += ('cell crystal a=%s b=%s c=%s alpha=%s beta=%s gamma=%s %s\n' % (a, b, c, alpha, beta, gamma, scale))
        if options.input: control += ('input %s\n' % ParsePluginCall(options.input))
        if options.output: control += ('output %s\n' % ParsePluginCall(options.output))
        if options.use:
           name, useblock = ParsePluginBlock(options.use)
           control += (useblock+'\n'+('%s %s\n' % (self.use_hint, name)))
        if options.cellmanager:
           name, useblock = ParsePluginBlock(options.cellmanager)
           control += (useblock+'\n'+('cellmanager %s\n' % name))
        return control

