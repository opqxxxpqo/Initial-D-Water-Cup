export const DEFAULT_COLORS = {
  background: '#000000',
  water: '#777777',
  surface: '#666666',
  waterHighlight: '#ffffff',
  droplet: '#f0f0f0',
  glass: '#a0a0a0',
  outline: '#757575',
  rim: '#e0e0e0',
  reflection: '#f0f0f0',
  base: '#505050',
  baseShadow: '#333333',
  secondary: '#ababab',
} as const;

export type ColorKey = keyof typeof DEFAULT_COLORS;
export type ColorScheme = Record<ColorKey, string>;
export const COLOR_LABELS: Record<ColorKey, string> = {
  background: '画面背景', water: '水体', surface: '水面',
  waterHighlight: '水面高光', droplet: '水滴', glass: '杯壁',
  outline: '杯身描边', rim: '杯口亮边', reflection: '玻璃高光',
  base: '杯底', baseShadow: '杯底暗部', secondary: '次级反光',
};

// This visual revision starts with a monochrome palette; v1 remains stored for rollback.
const STORAGE_KEY = 'dont-spill-colors-v2';
const isHex = (value: unknown): value is string =>
  typeof value === 'string' && /^#[0-9a-f]{6}$/i.test(value);

export function loadColors(): ColorScheme {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? '{}') as Record<string, unknown>;
    const result: ColorScheme = { ...DEFAULT_COLORS };
    for (const key of Object.keys(DEFAULT_COLORS) as ColorKey[]) {
      if (isHex(saved[key])) result[key] = saved[key];
    }
    return result;
  } catch { return { ...DEFAULT_COLORS }; }
}

export function saveColors(colors: ColorScheme): void {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(colors)); } catch { /* Preview still works. */ }
}

export { isHex };

