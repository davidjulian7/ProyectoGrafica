# Genera las etiquetas del HUD (nombre de lo que apunta la mira) y la ayuda de controles.
#   python herramientas/genera_etiquetas.py src/bin/textures/menu
import sys, os, unicodedata
from PIL import Image, ImageDraw, ImageFont
OUT = sys.argv[1] if len(sys.argv) > 1 else 'out'
os.makedirs(OUT + '/etiquetas', exist_ok=True)
FD = '/usr/share/fonts/opentype/noto/'
def font(w, s): return ImageFont.truetype(FD + f'NotoSansCJK-{w}.ttc', s, index=0)
SS = 2

# etiqueta (igual que en zonas.h) -> (texto bonito, categoria)
ETIQUETAS = {
    'Roca': ('Roca', 'ESCENARIO'), 'Coral': ('Coral', 'ARRECIFE'), 'Alga': ('Alga', 'ARRECIFE'),
    'Almeja gigante': ('Almeja gigante', 'FAUNA'), 'Cangrejo ermitano': ('Cangrejo ermitaño', 'FAUNA'),
    'Caballito de mar': ('Caballito de mar', 'FAUNA'), 'Pez payaso': ('Pez payaso', 'FAUNA'),
    'Pez cirujano azul': ('Pez cirujano azul', 'FAUNA'), 'Pez dragon': ('Pez dragón', 'FAUNA'),
    'Atun': ('Atún', 'FAUNA'), 'Tortuga marina': ('Tortuga marina', 'FAUNA'), 'Mantarraya': ('Mantarraya', 'FAUNA'),
    'Manati': ('Manatí', 'FAUNA'), 'Plataforma de hielo': ('Plataforma de hielo', 'HÁBITAT'), 'Iceberg': ('Iceberg', 'HÁBITAT'),
    'Ballena azul': ('Ballena azul', 'FAUNA'), 'Beluga': ('Beluga', 'FAUNA'), 'Foca': ('Foca', 'FAUNA'),
    'Pinguino': ('Pingüino', 'FAUNA'), 'Oso polar': ('Oso polar', 'FAUNA'), 'Morsa': ('Morsa', 'FAUNA'),
    'Krill': ('Krill', 'FAUNA'), 'Bacalao': ('Bacalao', 'FAUNA'), 'Submarino hundido': ('Submarino hundido', 'NAUFRAGIO'),
    'Ruinas': ('Ruinas sumergidas', 'NAUFRAGIO'), 'Lata de aluminio': ('Lata de aluminio', 'BASURA'),
    'Lata aplastada': ('Lata aplastada', 'BASURA'), 'Llanta': ('Llanta', 'BASURA'),
    'Bolsa de plastico': ('Bolsa de plástico', 'BASURA'), 'Pez leon (invasor)': ('Pez león', 'ESPECIE INVASORA'),
    'Barracuda': ('Barracuda', 'FAUNA'), 'Tiburon': ('Tiburón', 'FAUNA'), 'Vaquita marina': ('Vaquita marina', 'EN PELIGRO'),
}
COLORES = {'BASURA': (255, 150, 110), 'ESPECIE INVASORA': (255, 190, 90), 'EN PELIGRO': (255, 120, 120)}

def archivo(et):   # misma regla que MenuInicio::ArchivoEtiqueta()
    s = ''.join(c for c in unicodedata.normalize('NFD', et.lower()) if unicodedata.category(c) != 'Mn')
    return ''.join('_' if c == ' ' else c for c in s if c.isalnum() or c == ' ')

for et, (texto, cat) in ETIQUETAS.items():
    W, H = 640, 120
    im = Image.new('RGBA', (W * SS, H * SS), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    acento = COLORES.get(cat, (140, 230, 255))
    f1, f2, f3 = font('Bold', 22 * SS), font('Black', 44 * SS), font('Regular', 22 * SS)
    sub = 'Clic para recogerla' if cat == 'BASURA' else ''
    tw = max(d.textlength(texto, font=f2), d.textlength(cat, font=f1), d.textlength(sub, font=f3) if sub else 0)
    pw = tw + 56 * SS; x0 = (W * SS - pw) / 2
    d.rounded_rectangle([x0, 0, x0 + pw, H * SS - 2], radius=18 * SS, fill=(4, 18, 30, 175), outline=acento + (200,), width=2 * SS)
    d.text((W * SS / 2 - d.textlength(cat, font=f1) / 2, 8 * SS), cat, font=f1, fill=acento + (255,))
    d.text((W * SS / 2 - d.textlength(texto, font=f2) / 2, 32 * SS), texto, font=f2, fill=(255, 255, 255, 255))
    if sub: d.text((W * SS / 2 - d.textlength(sub, font=f3) / 2, 86 * SS), sub, font=f3, fill=(220, 230, 235, 255))
    im.resize((W, H), Image.LANCZOS).save(f'{OUT}/etiquetas/{archivo(et)}.png')

# ayuda de controles dentro de las zonas
W, H = 1900, 70
im = Image.new('RGBA', (W * SS, H * SS), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
fk, ft = font('Bold', 26 * SS), font('Medium', 27 * SS); x = 10 * SS; y = 14 * SS
def key(t):
    global x
    tw = d.textlength(t, font=fk); p = 14 * SS
    d.rounded_rectangle([x, y - 4 * SS, x + tw + 2 * p, y + 40 * SS], radius=8 * SS, fill=(4, 18, 30, 160), outline=(160, 225, 245, 255), width=2 * SS)
    d.text((x + p, y), t, font=fk, fill=(255, 255, 255, 255)); x += tw + 2 * p + 8 * SS
def txt(t):
    global x
    d.text((x, y), t, font=ft, fill=(225, 240, 245, 255)); x += d.textlength(t, font=ft) + 30 * SS
key('W'); key('A'); key('S'); key('D'); txt('nadar'); key('Espacio'); key('Ctrl'); txt('subir / bajar'); key('Shift'); txt('aletear')
key('F'); txt('linterna'); key('Clic'); txt('interactuar'); key('Esc'); txt('menú')
bb = im.getbbox(); c = im.crop(bb); im = Image.new('RGBA', (W * SS, H * SS), (0, 0, 0, 0)); im.alpha_composite(c, ((W * SS - c.width) // 2, (H * SS - c.height) // 2))
im.resize((W, H), Image.LANCZOS).save(f'{OUT}/hud_controles.png')
print('ok', len(ETIQUETAS))
