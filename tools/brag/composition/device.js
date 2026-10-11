// 3D-модель IMBA TV по фотографиям для вступления: корпус влетает, включается, камера уходит в экран.
// Матовый чёрный пластик (3D-печать, сверху мелкая фактура, по бокам слои), окно экрана смещено
// к задней грани, круглая кнопка спереди почти заподлицо, резиновый язычок USB-C слева, на дне велкро.
// Размеры в миллиметрах, ось Y вверх, перед - +Z.
import * as THREE from 'three';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

const W = 39, D = 37, H = 13.5, R = 5;   // рамка слева и справа от окна 3,5 мм
const BOTTOM = 1.2;                             // слой велкро
const WIN = { w: 32, d: 21, r: 2.2, z: -2.5 };  // окно экрана, центр смещён назад
export const SCREEN_Y = H - 1.55;               // плоскость экрана
export const WIN_Z = WIN.z;
const PAPER = 0xf3f2ed;

let seed = 12345;
const rnd = () => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed / 0x7fffffff; };

function roundedRect(w, h, r, cx = 0, cy = 0, hole = false) {
  const s = hole ? new THREE.Path() : new THREE.Shape();
  const x0 = cx - w / 2, y0 = cy - h / 2, x1 = cx + w / 2, y1 = cy + h / 2;
  s.moveTo(x0 + r, y0);
  s.lineTo(x1 - r, y0); s.quadraticCurveTo(x1, y0, x1, y0 + r);
  s.lineTo(x1, y1 - r); s.quadraticCurveTo(x1, y1, x1 - r, y1);
  s.lineTo(x0 + r, y1); s.quadraticCurveTo(x0, y1, x0, y1 - r);
  s.lineTo(x0, y0 + r); s.quadraticCurveTo(x0, y0, x0 + r, y0);
  return s;
}

// плита по контуру: снизу y0, высота h, фаска b; форма в плоскости XZ (shape y = -z)
function slab(shape, y0, h, b, segs = 3) {
  const g = new THREE.ExtrudeGeometry(shape, { depth: h - 2 * b, bevelEnabled: b > 0, bevelThickness: b, bevelSize: b, bevelSegments: segs, curveSegments: 20 });
  g.rotateX(-Math.PI / 2);
  g.translate(0, y0 + b, 0);
  return g;
}

function canvasTex(w, h, draw, repeat = [1, 1]) {
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  draw(c.getContext('2d'), w, h);
  const t = new THREE.CanvasTexture(c);
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  t.repeat.set(...repeat);
  t.anisotropy = 16;
  return t;
}

// мелкая фактура верхней поверхности печати
function fuzzTexture() {
  return canvasTex(512, 512, (g, w, h) => {
    g.fillStyle = '#808080'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 9000; i++) {
      const v = Math.floor(70 + rnd() * 120);
      g.fillStyle = `rgb(${v},${v},${v})`;
      g.beginPath(); g.arc(rnd() * w, rnd() * h, 1 + rnd() * 2.6, 0, 7); g.fill();
    }
    g.filter = 'blur(0.8px)'; g.drawImage(g.canvas, 0, 0); g.filter = 'none';
  }, [1 / 14, 1 / 14]);
}

// слои печати по 0,2 мм
function layerTexture() {
  return canvasTex(4, 32, (g, w, h) => {
    for (let y = 0; y < h; y++) {
      const v = Math.round(128 + 110 * Math.sin(y / h * Math.PI * 2));
      g.fillStyle = `rgb(${v},${v},${v})`; g.fillRect(0, y, w, 1);
    }
  }, [1, 5]);
}

// бумага с сеткой, как коврик для резки на фото
function paperTexture() {
  return canvasTex(4000, 4000, (g, w, h) => {
    g.fillStyle = '#f3f2ed'; g.fillRect(0, 0, w, h);
    for (let i = 0; i <= 100; i++) {
      const major = i % 5 === 0;
      g.strokeStyle = major ? 'rgba(29,29,27,0.16)' : 'rgba(29,29,27,0.07)';
      g.lineWidth = major ? 2.6 : 1.6;
      const p = i * 40;
      g.beginPath(); g.moveTo(p, 0); g.lineTo(p, h); g.stroke();
      g.beginPath(); g.moveTo(0, p); g.lineTo(w, p); g.stroke();
    }
  });
}

export function createDevice(canvas, oledCanvas) {
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true, preserveDrawingBuffer: true });
  renderer.setPixelRatio(1);
  renderer.setSize(canvas.width, canvas.height, false);
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;

  const scene = new THREE.Scene();
  scene.environment = new THREE.PMREMGenerator(renderer).fromScene(new RoomEnvironment(), 0.04).texture;
  scene.fog = new THREE.Fog(PAPER, 260, 700);

  scene.add(new THREE.HemisphereLight(0xffffff, 0xd8d4c8, 0.35));
  const key = new THREE.DirectionalLight(0xffffff, 1.7);
  key.position.set(-60, 120, 70);
  key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  Object.assign(key.shadow.camera, { left: -80, right: 80, top: 80, bottom: -80, near: 10, far: 400 });
  key.shadow.bias = -0.0004;
  scene.add(key);
  const rim = new THREE.DirectionalLight(0xffffff, 2.4);
  rim.position.set(50, 60, -110);
  scene.add(rim);
  const fill = new THREE.DirectionalLight(0xfff6e8, 0.7);
  fill.position.set(90, 40, 80);
  scene.add(fill);

  // пол: бумага с сеткой без освещения (цвет как у листа) и мягкая тень
  const paper = paperTexture();
  paper.colorSpace = THREE.SRGBColorSpace;
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(1000, 1000), new THREE.MeshBasicMaterial({ map: paper, toneMapped: false }));
  floor.rotation.x = -Math.PI / 2;
  floor.position.y = -0.01;
  scene.add(floor);
  const shadowPlane = new THREE.Mesh(new THREE.PlaneGeometry(600, 600), new THREE.ShadowMaterial({ opacity: 0.22 }));
  shadowPlane.rotation.x = -Math.PI / 2;
  shadowPlane.receiveShadow = true;
  scene.add(shadowPlane);
  const contact = new THREE.Mesh(new THREE.PlaneGeometry(W * 1.9, D * 1.9), new THREE.MeshBasicMaterial({
    map: canvasTex(256, 256, (g, w, h) => {
      const grd = g.createRadialGradient(w / 2, h / 2, 10, w / 2, h / 2, w / 2);
      grd.addColorStop(0, 'rgba(0,0,0,0.55)'); grd.addColorStop(0.55, 'rgba(0,0,0,0.25)'); grd.addColorStop(1, 'rgba(0,0,0,0)');
      g.fillStyle = grd; g.fillRect(0, 0, w, h);
    }), transparent: true, depthWrite: false, toneMapped: false,
  }));
  contact.rotation.x = -Math.PI / 2;
  contact.position.y = 0.02;
  scene.add(contact);

  // матовый чёрный пластик
  const fuzz = fuzzTexture(), layers = layerTexture();
  const plaTop = new THREE.MeshStandardMaterial({ color: 0x0d0d0d, roughness: 0.9, bumpMap: fuzz, bumpScale: 4, envMapIntensity: 0.55 });
  const plaSide = new THREE.MeshStandardMaterial({ color: 0x0c0c0c, roughness: 0.78, bumpMap: layers, bumpScale: 0.8, envMapIntensity: 0.6 });
  const rubber = new THREE.MeshStandardMaterial({ color: 0x1b1b1b, roughness: 0.92 });
  const shadowed = m => { m.castShadow = true; m.receiveShadow = true; return m; };

  const device = new THREE.Group();
  scene.add(device);

  // велкро на дне
  const velcroTex = canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#1a1a1a'; g.fillRect(0, 0, w, h);
    for (let y = 0; y < h; y += 8) for (let x = 0; x < w; x += 8) {
      const v = 40 + Math.floor(rnd() * 30);
      g.fillStyle = `rgb(${v},${v},${v})`;
      g.beginPath(); g.ellipse(x + 4, y + 4, 3.2, 2.4, 0.6, 0, 7); g.fill();
    }
  }, [1 / 4, 1 / 4]);
  velcroTex.colorSpace = THREE.SRGBColorSpace;
  const velcroMat = new THREE.MeshStandardMaterial({ color: 0x1c1c1c, roughness: 0.95, bumpMap: velcroTex, bumpScale: 1.5, map: velcroTex });
  device.add(shadowed(new THREE.Mesh(slab(roundedRect(W - 2.2, D - 2.2, R - 1.1), 0, 1.3, 0.3, 2), [velcroMat, velcroMat])));

  // корпус: цельная оболочка, верхнее ребро скруглено, окно экрана
  const shellShape = roundedRect(W - 2, D - 2, R - 1);
  shellShape.holes.push(roundedRect(WIN.w + 2, WIN.d + 2, WIN.r + 1, 0, -WIN.z, true));
  device.add(shadowed(new THREE.Mesh(slab(shellShape, BOTTOM, H - BOTTOM, 1.0, 4), [plaTop, plaSide])));

  // экран: OLED под стеклом, картинку рисует композиция в oledCanvas (без бликов и свечения)
  const oledTex = new THREE.CanvasTexture(oledCanvas);
  oledTex.colorSpace = THREE.SRGBColorSpace;
  oledTex.anisotropy = 16;
  const screen = new THREE.Mesh(new THREE.PlaneGeometry(WIN.w + 1, WIN.d + 1), new THREE.MeshBasicMaterial({ map: oledTex, toneMapped: false }));
  screen.rotation.x = -Math.PI / 2;
  screen.position.set(0, SCREEN_Y, WIN.z);
  device.add(screen);
  const well = new THREE.Mesh(new RoundedBoxGeometry(WIN.w + 1.4, 1.4, WIN.d + 1.4, 2, 0.3), new THREE.MeshStandardMaterial({ color: 0x050505, roughness: 0.6 }));
  well.position.set(0, H - 2.3, WIN.z);
  device.add(well);

  // кнопка спереди справа, почти заподлицо, вокруг тонкий зазор
  const button = new THREE.Group();
  const cap = shadowed(new THREE.Mesh(new THREE.CylinderGeometry(2.5, 2.5, 1.0, 48), new THREE.MeshStandardMaterial({ color: 0x161616, roughness: 0.8, envMapIntensity: 0.5 })));
  cap.rotation.x = Math.PI / 2;
  cap.position.z = -0.25;
  const gap = new THREE.Mesh(new THREE.RingGeometry(2.5, 2.85, 48), new THREE.MeshBasicMaterial({ color: 0x030303 }));
  gap.position.z = 0.02;
  button.add(cap, gap);
  button.position.set(11, 6.6, D / 2);
  device.add(button);

  // язычок USB-C слева: приклеен на дне, через ребро заходит в разъём
  const plug = new THREE.Group();
  const band = shadowed(new THREE.Mesh(new RoundedBoxGeometry(1.1, 8.6, 6.2, 4, 0.5), rubber));
  band.position.set(-0.45, 4.3, 0);
  const foot = shadowed(new THREE.Mesh(new RoundedBoxGeometry(5, 1.0, 6.2, 3, 0.45), rubber));
  foot.position.set(1.6, -0.1, 0);
  plug.add(band, foot);
  plug.position.set(-W / 2, 0.4, 4);
  device.add(plug);

  const camera = new THREE.PerspectiveCamera(28, canvas.width / canvas.height, 1, 2000);
  const buttonZ = button.position.z;

  // state: { cam: {pos, target, fov, up}, device: {x, y, z, rx, ry, rz}, press }
  function render(s) {
    camera.fov = s.cam.fov ?? 28;
    camera.up.set(...(s.cam.up ?? [0, 1, 0])).normalize();
    camera.position.set(...s.cam.pos);
    camera.lookAt(new THREE.Vector3(...s.cam.target));
    camera.updateProjectionMatrix();
    const d = s.device ?? {};
    device.position.set(d.x ?? 0, d.y ?? 0, d.z ?? 0);
    device.rotation.set(d.rx ?? 0, d.ry ?? 0, d.rz ?? 0, 'YXZ');
    button.position.z = buttonZ - 0.35 * (s.press ?? 0);
    contact.position.set(device.position.x, 0.02, device.position.z);
    contact.rotation.z = device.rotation.y;
    contact.material.opacity = Math.max(0, 1 - device.position.y / 25);
    oledTex.needsUpdate = true;
    renderer.render(scene, camera);
  }
  return { render };
}
