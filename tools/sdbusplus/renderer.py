class Renderer(object):
    def __init__(self, **kwargs):
        super(Renderer, self).__init__(**kwargs)

    def render(self, loader, template, **kwargs):
        t = loader.get_template(template)
        return t.render(loader=loader, **kwargs)
