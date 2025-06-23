const searchInput = document.getElementById('searchInput');
const suggestions = document.getElementById('suggestions');
const loading = document.getElementById('loading');

// Autocompletado (puede eliminarse si no lo necesitas)
searchInput.addEventListener('input', async function () {
  const query = this.value.trim();

  if (query.length < 2) {
    suggestions.style.display = 'none';
    loading.style.display = 'none';
    return;
  }

  loading.style.display = 'block';

  try {
    const response = await fetch(`/api/suggest?q=${encodeURIComponent(query)}`);
    const data = await response.json();

    if (data.length > 0) {
      suggestions.innerHTML = data.map(item =>
        `<div class="suggestion-item">${item}</div>`
      ).join('');

      document.querySelectorAll('.suggestion-item').forEach(item => {
        item.addEventListener('click', function () {
          searchInput.value = this.textContent;
          suggestions.style.display = 'none';
        });
      });

      suggestions.style.display = 'block';
    } else {
      suggestions.style.display = 'none';
    }
  } catch (error) {
    console.error('Error fetching suggestions:', error);
  }

  loading.style.display = 'none';
});

// Ocultar sugerencias al hacer clic fuera
document.addEventListener('click', function (e) {
  if (e.target !== searchInput) {
    suggestions.style.display = 'none';
  }
});

// 🔻 ❌ Eliminamos esta redirección conflictiva:
// document.getElementById('searchButton').addEventListener('click', function () {
//   const query = searchInput.value.trim();
//   if (query) {
//     window.location.href = `/index.html?q=${encodeURIComponent(query)}`;
//   }
// });

// Mantenemos enter como disparador del botón, pero no redireccionamos
searchInput.addEventListener('keypress', function (e) {
  if (e.key === 'Enter') {
    document.getElementById('searchButton').click();
  }
});
