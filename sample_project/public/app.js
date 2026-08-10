// API Base URL
const API_URL = '/api';

// ---------------------------------------------------------
// BLOG HOMEPAGE LOGIC (index.html)
// ---------------------------------------------------------
async function fetchPosts() {
    const grid = document.getElementById('posts-grid');
    if (!grid) return;

    try {
        const response = await fetch(`${API_URL}/posts`);
        const posts = await response.json();
        
        grid.innerHTML = '';
        
        if (posts.length === 0) {
            grid.innerHTML = '<p class="loading" style="color:var(--text-primary)">No posts found. Be the first to publish one!</p>';
            return;
        }

        posts.forEach(post => {
            const card = document.createElement('article');
            card.className = 'glass-card post-card';
            
            // Format content to maintain paragraphs
            const contentHtml = post.content.replace(/\n/g, '<br>');
            
            card.innerHTML = `
                <h3>${escapeHtml(post.title)}</h3>
                <div class="post-meta">${escapeHtml(post.created_at)}</div>
                <div class="post-content">${contentHtml}</div>
            `;
            grid.appendChild(card);
        });
    } catch (error) {
        grid.innerHTML = '<p class="error loading">Error fetching posts from server.</p>';
    }
}

// ---------------------------------------------------------
// ADMIN PANEL LOGIC (admin.html)
// ---------------------------------------------------------
function initAdmin() {
    const loginSection = document.getElementById('login-section');
    const dashboardSection = document.getElementById('dashboard-section');
    const loginForm = document.getElementById('login-form');
    const postForm = document.getElementById('post-form');
    const logoutBtn = document.getElementById('logout-btn');

    // Check if already logged in
    const token = localStorage.getItem('jwt_token');
    if (token) {
        showDashboard();
    }

    // Login Form Submit
    if (loginForm) {
        loginForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            const errorDiv = document.getElementById('login-error');

            try {
                const response = await fetch(`${API_URL}/login`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ username, password })
                });

                if (response.ok) {
                    const data = await response.json();
                    localStorage.setItem('jwt_token', data.token);
                    errorDiv.classList.add('hidden');
                    showDashboard();
                } else {
                    errorDiv.classList.remove('hidden');
                }
            } catch (err) {
                errorDiv.textContent = "Network error";
                errorDiv.classList.remove('hidden');
            }
        });
    }

    // Post Form Submit
    if (postForm) {
        postForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const title = document.getElementById('post-title').value;
            const content = document.getElementById('post-content').value;
            const successDiv = document.getElementById('post-success');
            const errorDiv = document.getElementById('post-error');
            const token = localStorage.getItem('jwt_token');

            try {
                const response = await fetch(`${API_URL}/posts`, {
                    method: 'POST',
                    headers: { 
                        'Content-Type': 'application/json',
                        'Authorization': `Bearer ${token}` 
                    },
                    body: JSON.stringify({ title, content })
                });

                if (response.ok) {
                    successDiv.classList.remove('hidden');
                    errorDiv.classList.add('hidden');
                    postForm.reset();
                    setTimeout(() => successDiv.classList.add('hidden'), 3000);
                } else if (response.status === 401) {
                    // Token expired or invalid
                    logout();
                } else {
                    errorDiv.classList.remove('hidden');
                }
            } catch (err) {
                errorDiv.textContent = "Network error";
                errorDiv.classList.remove('hidden');
            }
        });
    }

    // Logout
    if (logoutBtn) {
        logoutBtn.addEventListener('click', logout);
    }

    function showDashboard() {
        loginSection.classList.add('hidden');
        dashboardSection.classList.remove('hidden');
        logoutBtn.classList.remove('hidden');
        document.querySelector('.admin-container').style.maxWidth = '1000px';
    }

    function logout() {
        localStorage.removeItem('jwt_token');
        loginSection.classList.remove('hidden');
        dashboardSection.classList.add('hidden');
        logoutBtn.classList.add('hidden');
        document.querySelector('.admin-container').style.maxWidth = '500px';
    }
}

// Utility to prevent XSS in simple displays
function escapeHtml(unsafe) {
    return (unsafe||"")
         .replace(/&/g, "&amp;")
         .replace(/</g, "&lt;")
         .replace(/>/g, "&gt;")
         .replace(/"/g, "&quot;")
         .replace(/'/g, "&#039;");
}

// Run on page load
if (window.location.pathname.endsWith('index.html') || window.location.pathname === '/') {
    fetchPosts();
}
