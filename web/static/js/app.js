const state = {
    dashboard: null,
    exhibits: []
};

const sections = document.querySelectorAll(".page-section");
const navItems = document.querySelectorAll(".nav-item");
const pageTitle = document.getElementById("page-title");

const modalOverlay = document.getElementById("modal-overlay");
const modalContent = document.getElementById("modal-content");
const modalClose = document.getElementById("modal-close");

const toast = document.getElementById("toast");


function showToast(message) {
    toast.textContent = message;
    toast.classList.add("visible");

    setTimeout(() => {
        toast.classList.remove("visible");
    }, 2500);
}


function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}


function showSection(sectionName) {

    sections.forEach(section => {
        section.classList.remove("active-section");
    });

    navItems.forEach(item => {
        item.classList.remove("active");
    });

    const section = document.getElementById(sectionName);

    if (section) {
        section.classList.add("active-section");
    }

    const activeNav = document.querySelector(
        `[data-section="${sectionName}"]`
    );

    if (activeNav) {
        activeNav.classList.add("active");
    }

    const titles = {
        dashboard: "Dashboard",
        exhibits: "Exhibit Management",
        tickets: "Ticket Management",
        analytics: "Museum Analytics"
    };

    pageTitle.textContent = titles[sectionName] || "Dashboard";

    if (sectionName === "exhibits") {
        loadExhibits();
    }

    if (sectionName === "analytics") {
        updateAnalytics();
    }
}


/* ---------------- Dashboard ---------------- */

async function loadDashboard() {

    try {

        const response = await fetch("/api/dashboard");
        const data = await response.json();

        if (!response.ok || data.success === false) {
            throw new Error(
                data.error || "Unable to load dashboard."
            );
        }

        state.dashboard = data;

        document.getElementById("exhibits-count").textContent =
            data.exhibits;

        document.getElementById("visitors-count").textContent =
            data.visitors;

        document.getElementById("tickets-count").textContent =
            data.tickets;

        document.getElementById("revenue-value").textContent =
            `$${Number(data.revenue).toFixed(2)}`;

        updateAnalytics();

    } catch (error) {

        console.error(error);
        showToast(error.message);

    }
}


/* ---------------- Exhibits ---------------- */

async function loadExhibits() {

    const result = document.getElementById("exhibit-result");

    if (!result) {
        return;
    }

    result.innerHTML = `
        <div class="empty-state">
            Loading exhibit catalogue...
        </div>
    `;

    try {

        const response = await fetch("/api/exhibits");
        const data = await response.json();

        if (!response.ok || data.success === false) {
            throw new Error(
                data.error || "Unable to load exhibits."
            );
        }

        state.exhibits = data.exhibits || [];

        renderExhibitTable();

    } catch (error) {

        result.innerHTML = `
            <div class="empty-state">
                ${escapeHtml(error.message)}
            </div>
        `;

    }
}


function renderExhibitTable() {

    const result = document.getElementById("exhibit-result");

    if (!result) {
        return;
    }

    if (state.exhibits.length === 0) {

        result.innerHTML = `
            <div class="empty-state">
                No exhibits have been registered yet.
            </div>
        `;

        return;
    }

    result.innerHTML = `

        <div class="exhibit-table-wrapper">

            <table class="exhibit-table">

                <thead>

                    <tr>
                        <th>ID</th>
                        <th>Exhibit</th>
                        <th>Category</th>
                        <th>Visitors</th>
                        <th>Action</th>
                    </tr>

                </thead>

                <tbody>

                    ${state.exhibits.map(exhibit => `

                        <tr>

                            <td>
                                <span class="id-badge">
                                    ${exhibit.id}
                                </span>
                            </td>

                            <td>
                                <strong>
                                    ${escapeHtml(exhibit.name)}
                                </strong>
                            </td>

                            <td>
                                <span class="category-badge">
                                    ${escapeHtml(exhibit.category)}
                                </span>
                            </td>

                            <td>
                                ${exhibit.visitors}
                            </td>

                            <td>

                                <button
                                    class="table-action"
                                    onclick="confirmDeleteExhibit(
                                        ${exhibit.id},
                                        '${escapeHtml(exhibit.name)}'
                                    )"
                                >
                                    Delete
                                </button>

                            </td>

                        </tr>

                    `).join("")}

                </tbody>

            </table>

        </div>
    `;
}


async function confirmDeleteExhibit(id, name) {

    const confirmed = window.confirm(
        `Delete "${name}" (Exhibit #${id})?\n\n` +
        "This action cannot be undone."
    );

    if (!confirmed) {
        return;
    }

    try {

        const response = await fetch(
            `/api/exhibits/${id}/delete`,
            {
                method: "POST"
            }
        );

        const data = await response.json();

        if (!response.ok || data.success === false) {
            throw new Error(
                data.error || "Unable to delete exhibit."
            );
        }

        showToast(
            `Exhibit #${id} deleted successfully.`
        );

        await loadExhibits();
        await loadDashboard();

    } catch (error) {

        showToast(error.message);

    }
}


/* ---------------- Add Exhibit ---------------- */

function showAddExhibitModal() {

    openModal(`

        <h3 class="form-title">
            Add New Exhibit
        </h3>

        <form id="add-exhibit-form">

            <div class="form-group">

                <label for="exhibit-name">
                    EXHIBIT NAME
                </label>

                <input
                    id="exhibit-name"
                    type="text"
                    maxlength="49"
                    placeholder="e.g. Ancient Egypt"
                    required
                >

            </div>


            <div class="form-group">

                <label for="exhibit-category">
                    CATEGORY
                </label>

                <input
                    id="exhibit-category"
                    type="text"
                    maxlength="29"
                    placeholder="e.g. History"
                    required
                >

            </div>


            <div class="form-actions">

                <button
                    type="button"
                    class="secondary-button"
                    id="cancel-form"
                >
                    Cancel
                </button>

                <button
                    type="submit"
                    class="primary-button"
                >
                    Create Exhibit
                </button>

            </div>

        </form>
    `);


    document
        .getElementById("cancel-form")
        .addEventListener(
            "click",
            closeModal
        );


    document
        .getElementById("add-exhibit-form")
        .addEventListener(
            "submit",
            submitExhibit
        );
}


async function submitExhibit(event) {

    event.preventDefault();

    const name =
        document
            .getElementById("exhibit-name")
            .value
            .trim();

    const category =
        document
            .getElementById("exhibit-category")
            .value
            .trim();

    if (!name || !category) {
        showToast(
            "Exhibit name and category are required."
        );
        return;
    }

    try {

        const response = await fetch(
            "/api/exhibits",
            {
                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    name,
                    category
                })
            }
        );

        const data = await response.json();

        if (!response.ok || data.success === false) {
            throw new Error(
                data.error || "Unable to create exhibit."
            );
        }

        closeModal();

        showToast(
            `Exhibit created successfully. ID: ${data.exhibitID}`
        );

        await loadExhibits();
        await loadDashboard();

    } catch (error) {

        showToast(error.message);

    }
}


/* ---------------- Search ---------------- */

async function searchExhibit() {

    const id =
        document
            .getElementById("search-exhibit-id")
            .value
            .trim();

    if (!id) {
        showToast("Enter an exhibit ID.");
        return;
    }

    const result =
        document.getElementById("exhibit-result");

    result.innerHTML = `
        <div class="empty-state">
            Searching...
        </div>
    `;

    try {

        const response =
            await fetch(`/api/exhibits/${id}`);

        const data =
            await response.json();

        if (!response.ok || data.found === false) {
            throw new Error(
                data.error || "Exhibit not found."
            );
        }

        result.innerHTML = `

            <div class="result-card">

                <h4>
                    ${escapeHtml(data.name)}
                </h4>

                <div class="result-row">
                    <span>Exhibit ID</span>
                    <strong>${data.id}</strong>
                </div>

                <div class="result-row">
                    <span>Category</span>
                    <strong>
                        ${escapeHtml(data.category)}
                    </strong>
                </div>

                <div class="result-row">
                    <span>Visitors</span>
                    <strong>
                        ${data.visitors}
                    </strong>
                </div>

            </div>
        `;

    } catch (error) {

        result.innerHTML = `
            <div class="empty-state">
                ${escapeHtml(error.message)}
            </div>
        `;

    }
}


/* ---------------- Tickets ---------------- */

function showPurchaseTicketModal() {

    openModal(`

        <h3 class="form-title">
            Record Ticket Sale
        </h3>

        <form id="ticket-form">

            <div class="form-group">

                <label for="ticket-exhibit">
                    EXHIBIT ID
                </label>

                <input
                    id="ticket-exhibit"
                    type="number"
                    min="1"
                    placeholder="e.g. 1"
                    required
                >

            </div>


            <div class="form-group">

                <label for="visitor-name">
                    VISITOR NAME
                </label>

                <input
                    id="visitor-name"
                    type="text"
                    maxlength="49"
                    placeholder="e.g. Michael Brown"
                    required
                >

            </div>


            <div class="form-group">

                <label for="ticket-price">
                    TICKET PRICE
                </label>

                <input
                    id="ticket-price"
                    type="number"
                    min="0.01"
                    step="0.01"
                    placeholder="25.00"
                    required
                >

            </div>


            <div class="form-actions">

                <button
                    type="button"
                    class="secondary-button"
                    id="cancel-ticket"
                >
                    Cancel
                </button>

                <button
                    type="submit"
                    class="primary-button"
                >
                    Complete Sale
                </button>

            </div>

        </form>
    `);


    document
        .getElementById("cancel-ticket")
        .addEventListener(
            "click",
            closeModal
        );


    document
        .getElementById("ticket-form")
        .addEventListener(
            "submit",
            submitTicket
        );
}


async function submitTicket(event) {

    event.preventDefault();

    const exhibit_id =
        document
            .getElementById("ticket-exhibit")
            .value;

    const visitor_name =
        document
            .getElementById("visitor-name")
            .value
            .trim();

    const price =
        document
            .getElementById("ticket-price")
            .value;

    try {

        const response = await fetch(
            "/api/tickets",
            {
                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    exhibit_id,
                    visitor_name,
                    price
                })
            }
        );

        const data = await response.json();

        if (!response.ok || data.success === false) {
            throw new Error(
                data.error || "Unable to process ticket."
            );
        }

        closeModal();

        showToast(
            `Ticket sold successfully. Ticket ID: ${data.ticketID}`
        );

        await loadDashboard();

        if (
            document
                .getElementById("exhibits")
                .classList
                .contains("active-section")
        ) {
            await loadExhibits();
        }

    } catch (error) {

        showToast(error.message);

    }
}


/* ---------------- Analytics ---------------- */

function updateAnalytics() {

    if (!state.dashboard) {
        return;
    }

    const {
        exhibits,
        visitors,
        tickets,
        revenue
    } = state.dashboard;

    const visitorRatio =
        tickets > 0
            ? (visitors / tickets).toFixed(2)
            : "0.00";

    const averageTicket =
        tickets > 0
            ? (revenue / tickets).toFixed(2)
            : "0.00";

    const ratioElement =
        document.getElementById("visitor-ratio");

    const averageElement =
        document.getElementById("average-ticket");

    const summary =
        document.getElementById("analytics-summary");

    if (ratioElement) {
        ratioElement.textContent =
            visitorRatio;
    }

    if (averageElement) {
        averageElement.textContent =
            `$${averageTicket}`;
    }

    if (summary) {

        summary.innerHTML = `

            <div class="result-card">

                <div class="result-row">
                    <span>Registered exhibits</span>
                    <strong>${exhibits}</strong>
                </div>

                <div class="result-row">
                    <span>Total visitors</span>
                    <strong>${visitors}</strong>
                </div>

                <div class="result-row">
                    <span>Tickets processed</span>
                    <strong>${tickets}</strong>
                </div>

                <div class="result-row">
                    <span>Total revenue</span>
                    <strong>
                        $${Number(revenue).toFixed(2)}
                    </strong>
                </div>

                <div class="result-row">
                    <span>Average ticket value</span>
                    <strong>
                        $${averageTicket}
                    </strong>
                </div>

            </div>
        `;
    }
}


/* ---------------- Modal ---------------- */

function openModal(content) {

    modalContent.innerHTML = content;

    modalOverlay.classList.add(
        "visible"
    );
}


function closeModal() {

    modalOverlay.classList.remove(
        "visible"
    );

    modalContent.innerHTML = "";
}


modalClose.addEventListener(
    "click",
    closeModal
);


modalOverlay.addEventListener(
    "click",
    event => {

        if (event.target === modalOverlay) {
            closeModal();
        }

    }
);


/* ---------------- Navigation ---------------- */

navItems.forEach(item => {

    item.addEventListener(
        "click",
        () => {
            showSection(
                item.dataset.section
            );
        }
    );

});


/* ---------------- Quick Actions ---------------- */

document
    .querySelectorAll("[data-action]")
    .forEach(button => {

        button.addEventListener(
            "click",
            () => {

                const action =
                    button.dataset.action;

                if (action === "add-exhibit") {
                    showAddExhibitModal();
                }

                if (action === "purchase-ticket") {
                    showPurchaseTicketModal();
                }

                if (action === "search-exhibit") {

                    showSection(
                        "exhibits"
                    );

                    setTimeout(() => {

                        const input =
                            document.getElementById(
                                "search-exhibit-id"
                            );

                        if (input) {
                            input.focus();
                        }

                    }, 100);

                }

            }
        );

    });


/* ---------------- Section Targets ---------------- */

document
    .querySelectorAll("[data-section-target]")
    .forEach(button => {

        button.addEventListener(
            "click",
            () => {

                showSection(
                    button.dataset.sectionTarget
                );

            }
        );

    });


/* ---------------- Dashboard Refresh ---------------- */

document
    .getElementById("refresh-dashboard")
    .addEventListener(
        "click",
        async () => {

            await loadDashboard();

            if (
                document
                    .getElementById("exhibits")
                    .classList
                    .contains("active-section")
            ) {
                await loadExhibits();
            }

        }
    );


/* ---------------- Exhibit Search ---------------- */

document
    .getElementById("search-exhibit-button")
    .addEventListener(
        "click",
        searchExhibit
    );


/* ---------------- Initial Load ---------------- */

loadDashboard();