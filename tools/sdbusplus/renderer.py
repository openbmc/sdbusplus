class Renderer(object):
    def __init__(self, **kwargs):
        super(Renderer, self).__init__()

    def render(self, loader, template, **kwargs):
        t = loader.get_template(template)
        post = kwargs.pop("post", lambda result: result)
        r = t.render(loader=loader, **kwargs)
        return post(r)
